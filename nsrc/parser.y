%{
#include <assert.h>
#include <stdio.h>

#include "asm.h"
#include "param.h"
#include "name.h"
#include "pdp8.h"
#include "error.h"
#include "parser.h"
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

		/* simple definition */
definition	: define_name initializer ';'

		/* vector definition */
		| define_name '[' {
			instr(".+1"); /* TODO: perhaps add this to pdp8.h */
			comment(NAMEFMT, $1.name);
		} vector_length ']' initializer ';' {
			int want, have;

			want = val($4.value);
			have = val($6.value);

			if (have < want)
				skip(want - have);
			else if (want == 0)
				warn($1.name, "zero length vector");
		}

		/* function definition */
		| define_name { newframe(&$1); } '(' parameters ')' statement {
			/* TODO: emit return */
			endframe();
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
			skip(1);
			if ($0.value != TOKEN)
				comment(NAMEFMT, $0.name);

			$$.value = 0;
		}
		| ival_list
		;

ival_list	: ival {
			if ($0.value != TOKEN)
				comment(NAMEFMT, $0.name);

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
		| '[' statement_list ']'
		| IF '(' expr ')' statement %prec ELSE
		| IF '(' expr ')' statement ELSE statement
		| WHILE '(' expr ')' statement
		| SWITCH '(' expr ')' statement		/* not original */
		| GOTO expr ';'
		| RETURN expr ';'
		| RETURN ';'
		| BREAK ';'
		| expr ';' { pop(&$1); }
		| ';'
		;

label		: NAME
		| CASE CONSTANT
		| DEFAULT
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

arguments	: /* empty */
		| argument_list
		;

argument_list	: expr
		| argument_list ',' expr
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
		| expr '(' arguments ')'
		| expr '[' expr ']' {
			lda(&$3);
			pop(&$3);
			tad(&$1);
			pop(&$1);
			push(&$$);
			assert(isrval($$.value));
			$$ = r2lval(&$$);
		}
		| expr INC {
			lda(&$2);
			isz(&$2);
			opr(NOP);
			pop(&$2);
			push(&$$);
		}
		| expr DEC {
			opr(STA);
			tad(&$2);
			dca(&$2);
			lda(&$2);
			opr(IAC);
			pop(&$2);
			push(&$$);
		}
		| INC expr {
			isz(&$2);
			opr(NOP);
			$$ = $2;
		}
		| DEC expr {
			opr(STA);
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
				/* TODO: perhaps add reify */
				push(&$2);
			}

			$$ = r2lval(&$2);
		}
		| '&' expr %prec INC {
			$$ = l2rval(&$2);
			if ($$.value = NOLVAL)
				error($2.name, "not an lvalue");

			/* TODO: YYERROR; */
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
		| expr '*' expr /* TODO */
		| expr '%' expr /* TODO */
		| expr '/' expr /* TODO */
		| expr '+' expr {
			lda(&$3);
			pop(&$3);
			tad(&$1);
			pop(&$1);
			push(&$$);
		}
		| expr '-' expr {
			lda(&$3);
			pop(&$3);
			opr(CIA);
			tad(&$1);
			pop(&$1);
			push(&$$);
		}
		| expr SHL expr
		| expr SHR expr
		| expr '<' expr
		| expr '>' expr
		| expr LE expr
		| expr GE expr
		| expr EQ expr
		| expr NE expr
		| expr '&' expr
		| expr '^' expr
		| expr '\\' expr
		| expr '?' expr ':' expr
		| expr '=' expr {
			lda(&$3);
			pop(&$3);
			dca(&$1);
			$$ = $1;
		}
		| expr ASMUL expr
		| expr ASMOD expr
		| expr ASDIV expr
		| expr ASADD expr
		| expr ASSUB expr
		| expr ASSHL expr
		| expr ASSHR expr
		| expr ASLT expr
		| expr ASGT expr
		| expr ASLE expr
		| expr ASGE expr
		| expr ASEQ expr
		| expr ASNE expr
		| expr ASAND expr
		| expr ASXOR expr
		| expr ASOR expr
		;

%%