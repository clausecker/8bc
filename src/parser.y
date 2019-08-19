%{
#include <stdio.h>

#include "param.h"
#include "pdp8.h"
#include "asm.h"
#include "codegen.h"
#include "name.h"
#include "data.h"
#include "error.h"
#include "parser.h"

/*
 * when we see function arguments, they are placed on argstack and
 * later emitted into the function call.  This is needed to support
 * nested function calls.
 */
static struct expr argstack[ARGSIZ];
static char narg = 0;

static void argpush(struct expr *);
static void docall(struct expr *, int);
static void docmp(struct expr *, struct expr *, struct expr *, int, int);
static void door(struct expr *, struct expr *, struct expr *, int, int);
static void doshift(struct expr *, struct expr *, struct expr *, int, int);

/* arguments for *, /, and % */
static struct expr factor = { RVALUE | 00010, "FACTOR" };
%}

%token	CONSTANT
%token	NAME
%token	AUTO		/* the AUTO keyword */
%token	EXTRN		/* the EXTRN keyword */
%token	CASE		/* the CASE keyword */
%token	IF		/* the IF keyword */
%token	ELSE		/* the ELSE keyword */
%token	WHILE		/* the WHILE keyword */
%token	SWITCH		/* the SWITCH keyword */
%token	GOTO		/* the GOTO keyword */
%token	RETURN		/* the RETURN keyword */
%token	BREAK		/* the BREAK keyword */
%token	DEFAULT		/* the DEFAULT keyword */

%token	ASOR		/* the =\ mark */
%token	ASXOR		/* the =^ mark */
%token	ASAND		/* the =& mark */
%token	ASEQ		/* the === mark */
%token	ASNE		/* the =!= mark */
%token	ASLT		/* the =< mark */
%token	ASLE		/* the =<= mark */
%token	ASGT		/* the => mark */
%token	ASGE		/* the =>= mark */
%token	ASSHL		/* the =<< mark */
%token	ASSHR		/* the =>> mark */
%token	ASADD		/* the =+ mark */
%token	ASSUB		/* the =- mark */
%token	ASMOD		/* the =% mark */
%token	ASMUL		/* the =* mark */
%token	ASDIV		/* the =/ mark */

%token	INC		/* the ++ mark */
%token	DEC		/* the -- mark */

%token	EQ		/* the == mark */
%token	NE		/* the != mark */
%token	LE		/* the <= mark */
%token	GE		/* the >= mark */
%token	SHL		/* the << mark */
%token	SHR		/* the >> mark */

%right ELSE
%right '=' ASOR ASXOR ASAND ASEQ ASNE ASLT ASLE ASGT ASGE ASSHL ASSHR ASADD ASSUB ASMOD ASMUL ASDIV
%right '?' ':'
%left '\\'
%left '^'
%left '&'
%nonassoc EQ NE
%nonassoc '<' '>' LE GE
%left SHL SHR
%left '-'
%left '+'
%left '%' '*' '/'
%right INC DEC '[' '(' '!'

%start	program

%%

program		: /* empty */
		| program definition { blank(); }
		;

definition	: define_name initializer ';'
		| define_name { newframe(&$1); } '(' parameters ')' statement { endframe(&$1); }
		| define_name { /* vector definition */
			instr(".+1");
			commentname($1.name);
		} '[' vector_length ']' initializer ';' {
			int want, have;

			want = val($4.value);
			have = val($6.value);

			if (have < want)
				advance(want - have);
			else if (want == 0)
				warn($1.name, "zero length vector");
		}
		;

define_name	: NAME {
			struct expr *e;

			/* TODO: detect redefinitions */
			e = define($1.name);
			putlabel(e);

			$$ = *e;
		}
		;

		/*
		 * In a simple definition, the token left from this one
		 * is a define_name, in a vector definition its a ].
		 * Use this to place a comment with the name of what we
		 * just defined in a simple definition.
		 */
initializer	: /* empty */ {
			emitc(0);
			if ($0.value != TOKEN)
				commentname($0.name);

			$$.value = 0;
		}
		| ival_list
		;

ival_list	: ival {
			if ($0.value != TOKEN)
				commentname($0.name);

			$$.value = 1;
		}
		| ival_list ',' ival { $$.value = $1.value + 1; }
		;

ival		: CONSTANT { emitr(&$1); }
		| NAME { emitl(define($1.name)); }
		;

vector_length	: /* empty */ { $$.value = RCONST | 0; }
		| CONSTANT
		;

parameters	: /* empty */
		| param_list
		;

param_list	: param
		| param_list ',' param
		;

param		: NAME {
			newparam(&$1);
			declare(&$1);
		}
		;

statement	: AUTO auto_list ';' statement
		| EXTRN extrn_list ';' statement
		| label ':' statement
		| { $$.value = beginscope(); } '[' statement_list ']' { endscope($1.value); }
		| IF if_control statement %prec ELSE {
			ldconst(0);
			putlabel(&$2);
		}
		| IF if_control statement ELSE {
			newlabel(&$$);
			ldconst(0);
			jmp(&$$);
			putlabel(&$2);
		} statement {
			ldconst(0);
			putlabel(&$5);
		}
		| WHILE {
			newlabel(&$$);
			ldconst(0);
			putlabel(&$$);
		} if_control statement {
			ldconst(0);
			jmp(&$2);
			putlabel(&$3);
		}
		| SWITCH '(' expr ')' statement	{ /* original: SWITCH expr statement */
			pop(&$3);
			error("SWITCH", "not implemented");
		}
		| GOTO expr ';' {
			ldconst(0);
			jmp(&$2);
			pop(&$2);
		}
		| RETURN expr ';' {
			lda(&$2);
			pop(&$2);
			ret();
		}
		| RETURN ';' { ret(); }
		| BREAK ';' { error("BREAK", "not implemented"); }
		| expr ';' { pop(&$1); }
		| ';'
		;

if_control	: '(' expr ')' {
			newlabel(&$$);
			lda(&$2);
			pop(&$2);
			opr(SNA | CLA);
			jmp(&$$);
		}
		;

label		: NAME {
			newlabel(&$1);
			declare(&$1);
			ldconst(0);
			putlabel(&$1);
		}
		| CASE CONSTANT { error("CASE", "not implemented"); }
		| DEFAULT { error("DEFAULT", "not implemented"); }
		;

statement_list	: /* empty */
		| statement_list statement
		;

auto_list	: auto_decl
		| auto_list ',' auto_decl
		;

auto_decl	: NAME {
			newauto(&$1);
			declare(&$1);
		}
		| NAME CONSTANT {
			newauto(&$1);
			declare(&$1);
			lda(&$2);
			dca(&$1);
		}
		;

extrn_list	: extrn_decl
		| extrn_list ',' extrn_decl
		;

extrn_decl	: NAME { declare(define($1.name)); }
		;

		/* $$.value is the number of arguments */
arguments	: /* empty */ { $$.value = 0; }
		| argument_list
		;

argument_list	: expr {
			argpush(&$1);
			$$.value = 1;
		}
		| argument_list ',' expr {
			argpush(&$3);
			$$.value = $1.value + 1;
		}
		;

expr		: NAME {
			struct expr *e;

			/* if $1 not found, declare as internal variable */
			e = lookup($1.name);
			if (e == NULL) {
				newlabel(&$1);
				declare(&$1);
				e = &$1;
			}

			$$ = *e;
		}
		| CONSTANT /* default action */
		| '(' expr ')' { $$ = $2; }
		| expr '(' arguments ')' {
			jms(&$1);
			docall(&$$, $3.value);
		}
		| expr '[' expr ']' {
			lda(&$3);
			pop(&$3);
			tad(&$1);
			pop(&$1);
			push(&$$);
			$$ = r2lval(&$$);
		}
		| expr INC {
			lda(&$1);
			isz(&$1);
			opr(NOP);
			pop(&$1);
			push(&$$);
		}
		| expr DEC {
			ldconst(07777);
			tad(&$1);
			dca(&$1);
			lda(&$1);
			opr(IAC);
			pop(&$1);
			push(&$$);
		}
		| INC expr {
			isz(&$2);
			opr(NOP);
			$$ = $2;
		}
		| DEC expr {
			ldconst(07777);
			tad(&$2);
			dca(&$2);
			$$ = $2;
		}
		| '+' expr %prec INC { $$ = $2; }
		| '-' expr %prec INC {
			lda(&$2);
			pop(&$2);
			opr(CIA);
			push(&$$);
		}
		| '*' expr %prec INC {
			/* perform a load to get an rvalue if needed */
			if (islval($2.value)) {
				lda(&$2);
				pop(&$2);
				forcepush(&$2);
			}

			$$ = r2lval(&$2);
		}
		| '&' expr %prec INC {
			$$ = l2rval(&$2);
			if ($$.value == NOLVAL)
				error($2.name, "not an lvalue");
		}
		| '^' expr %prec INC {
			lda(&$2);
			pop(&$2);
			opr(CMA);
			push(&$$);
		}
		| '!' expr {
			lda(&$2);
			pop(&$2);
			opr(SNA | CLA);
			opr(CLA | IAC);
			push(&$$);
		}
		| expr '*' expr {
			lda(&$3);
			pop(&$3);
			dca(&factor);
			lda(&$1);
			pop(&$1);
			acrandom();
			instr("MUL");
			push(&$$);
		}
		| expr ASMUL expr {
			lda(&$3);
			pop(&$3);
			dca(&factor);
			lda(&$1);
			acrandom();
			instr("MUL");
			dca(&$1);
			$$ = $1;
		}
		| expr '%' expr /* TODO */
		| expr ASMOD expr /* TODO */
		| expr '/' expr /* TODO */
		| expr ASDIV expr /* TODO */
		| expr '+' expr {
			lda(&$3);
			pop(&$3);
			tad(&$1);
			pop(&$1);
			push(&$$);
		}
		| expr ASADD expr {
			lda(&$3);
			pop(&$3);
			tad(&$1);
			dca(&$1);
			$$ = $1;
		}
		| expr '-' expr {
			lda(&$3);
			pop(&$3);
			opr(CIA);
			tad(&$1);
			pop(&$1);
			push(&$$);
		}
		| expr ASSUB expr {
			lda(&$3);
			pop(&$3);
			opr(CIA);
			tad(&$1);
			dca(&$1);
			$$ = $1;
		}
		| expr SHL expr { doshift(&$$, &$1, &$3, RAL, 0); }
		| expr ASSHL expr { doshift(&$$, &$1, &$3, RAL, 1); }
		| expr SHR expr { doshift(&$$, &$1, &$3, RAR, 0); }
		| expr ASSHR expr { doshift(&$$, &$1, &$3, RAR, 1); }
		| expr '<' expr { docmp(&$$, &$1, &$3, SZL, 0); }
		| expr ASLT expr { docmp(&$$, &$1, &$3, SZL, 1); }
		| expr '>' expr { docmp(&$$, &$1, &$3, SNL | SZA, 0); }
		| expr ASGT expr { docmp(&$$, &$1, &$3, SNL | SZA, 1); }
		| expr LE expr { docmp(&$$, &$1, &$3, SZL | SNA, 0); }
		| expr ASLE expr { docmp(&$$, &$1, &$3, SNA, 1); }
		| expr GE expr { docmp(&$$, &$1, &$3, SNL, 0); }
		| expr ASGE expr { docmp(&$$, &$1, &$3, SNL, 1); }
		| expr EQ expr { docmp(&$$, &$1, &$3, SNA, 0); }
		| expr ASEQ expr { docmp(&$$, &$1, &$3, SNA, 1); }
		| expr NE expr { docmp(&$$, &$1, &$3, SZA, 0); }
		| expr ASNE expr { docmp(&$$, &$1, &$3, SZA, 1); }
		| expr '&' expr {
			lda(&$3);
			pop(&$3);
			and(&$1);
			pop(&$1);
			push(&$$);
		}
		| expr ASAND expr {
			lda(&$3);
			pop(&$3);
			and(&$1);
			dca(&$1);
			$$ = $1;
		}
		| expr '^' expr { door(&$$, &$1, &$3, CMA | STL | RAL | IAC, 0); }
		| expr ASXOR expr { door(&$$, &$1, &$3, CMA | STL | RAL | IAC, 1); }
		| expr '\\' expr { door(&$$, &$1, &$3, CIA, 0); }
		| expr ASOR expr { door(&$$, &$1, &$3, CIA, 1); }
		| expr '?' expr ':' expr {
			opr(STA | CLL);
			tad(&$1); /* L = $1 != 0 */
			pop(&$1);
			opr(SNL | CLA);
			tad(&$5); /* won't set L */
			pop(&$5);
			opr(SZL); /* L is only set if $1 == 0 */
			tad(&$3);
			pop(&$3);
			push(&$$);
		}
		| expr '=' expr {
			lda(&$3);
			pop(&$3);
			dca(&$1);
			$$ = $1;
		}
		;

%%

/*
 * push e on the argument stack.  Signal an error if the
 * stack is full.
 */
static void
argpush(struct expr *e)
{
	if (narg >= ARGSIZ)
		fatal(e->name, "argument stack exhausted");

	/* manually spill unspillable cases */
	if (class(e->value) == LSTACK || class(e->value) == LVALUE) {
		lda(e);
		pop(e);
		push(e);
	}

	argstack[(int)narg++] = *e;
}

/*
 * Emit a function call with argc arguments and store the return
 * value in e.  The JMS instruction is not emitted.
 */
static void docall(struct expr *q, int argc)
{
	int i, arg0;

	arg0 = narg - argc;
	for (i = 0; i < argc; i++)
		emitl(&argstack[arg0 + i]);

	while (narg > arg0)
		pop(argstack + (int)--narg);

	push(q);
}

/*
 * Perform a comparison of a and b by predicate p.  If as is clear,
 * push the result to q and pop both a and b.  Otherwise deposit the
 * result in a, pop b, and copy a to q.
 */
static void
docmp(struct expr *q, struct expr *a, struct expr *b, int p, int as)
{
	lda(b);
	pop(b);
	/* only set up L if we have SZL or SNL */
	opr(p & 00020 ? CIA | STL : CIA);
	tad(a);
	if (!as)
		pop(a);
	opr(p | CLA);
	opr(CLA | IAC);
	if (as) {
		dca(a);
		*q = *a;
	} else
		push(q);
}

/*
 * Perform a bitwise or or xor of a and b.  If as is clear, pop both
 * a and b and store the result to q.  If as is set, pop only a, store
 * the result to b and copy a to q.
 *
 * If op is CIA, this performs an inclusive or by computing
 * a + b - (a & b).  If op is CMA | STL | RAL | IAC, this computes an
 * exclusive or by computing a + b - 2 * (a & b);
 */
static void
door(struct expr *q, struct expr *a, struct expr *b, int op, int as)
{
	/* compute $1 + $3 - ($1 & $3) */
	lda(b);
	and(a);
	opr(op);
	tad(b);
	pop(b);
	tad(a);
	if (as) {
		dca(a);
		*q = *a;
	} else {
		pop(a);
		push(q);
	}
}

/*
 * Perform a bitshift of a by b.  If as is clear, pop both a an d b and
 * store the result to q.  If as is set, pop only a, store the result to
 * b and copy a to q.
 *
 * If op is RAL, this performs a left shift.  If op is RAR, this
 * instead performs a right shift.
 */
static void
doshift(struct expr *q, struct expr *a, struct expr *b, int op, int as)
{
	if (isconst(b->value)) {
		static const unsigned short masks[2][11] = {
			07777, 07777, 07770, 07760, 07740, 07700, 07600, 07400, 07000, 06000, 07777,
			07777, 07777, 00777, 00377, 00177, 00077, 00037, 00017, 00007, 00003, 07777,
		};
		struct expr mask = { 0, "" }, zero = { RCONST | 0, "" };
		int v;

		v = val(b->value);
		pop(b);

		/* trivial cases */
		if (v == 0) {
			*q = *a;
			return;
		} else if (v >= 12) {
			if (as) {
				ldconst(0);
				dca(a);
				*q = *a;
			} else {
				pop(a);
				*q = zero;
			}

			return;
		}

		lda(a);
		if (!as)
			pop(a);

		/* manually unroll the loop */
		switch (v) {
		case 1:
			opr(CLL | op);
			break;

		case 2:
			opr(CLL | op);
			opr(CLL | op);
			break;

		case 3:
			opr(CLL | BSW | op);
			opr(op);
			break;

		case 4:
			opr(CLL | BSW | op);
			opr(BSW | op);
			break;

		case 5:
			opr(CLL | BSW | op);
			opr(BSW | op);
			opr(op);
			break;

		case 6:
			opr(BSW);
			break;

		case 7:
			opr(BSW);
			opr(CLL | op);
			break;

		case 8:
			opr(BSW);
			opr(CLL | BSW | op);
			break;

		case 9:
			opr(CLL | BSW | op ^ 00014);
			opr(BSW | op ^ 00014);
			break;

		case 10:
			opr(CLL | BSW | op ^ 00014);
			opr(op ^ 00014);
			break;

		case 11:
			opr(CLL | op ^ 00014);
			opr(CLA | op);
			break;
		}

		mask.value = RCONST | masks[op == RAR][v - 1];
		and(&mask);
		if (as) {
			dca(a);
			*q = *a;
		} else
			push(q);
	} else {
		struct expr again = { 0, "" }, end = { 0, "" }, counter;

		/*
		 * the loop goes like this:
		 *
		 * counter = ~b
		 * while (++counter)
		 *     a << = 1;
		 */
		newlabel(&again);
		newlabel(&end);

		lda(b);
		pop(b);
		opr(CMA);
		push(&counter);
		lda(a);
		jmp(&end);
		putlabel(&again);
		opr(CLL | op);
		putlabel(&end);
		isz(&counter);
		jmp(&again);
		pop(&counter);
		if (as) {
			dca(a);
			*q = *a;
		} else {
			pop(a);
			push(q);
		}
	}
}
