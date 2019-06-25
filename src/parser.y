%{
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "name.h"

extern int yyerror(const char *);
extern int yylex(void);

/* AC manipulation */
/*
 * ac is the value that is in the accumulator.  If AC contains a value
 * of type CONST, it is not actually loaded into AC until needed.  The
 * true content of AC in this case is 0.
 *
 * acstate says about the content of AC:
 * ACCLEAR: AC is clear (only if AC is of disposition CONST)
 * CONST | ...: AC holds that value (only if AC is of disposition CONST)
 * ACDIRTY: AC needs to be written back (only if AC is not of disposition CONST)
 * ACUNDEF: AC holds an unknown value
 */
enum { ACCLEAR = CONST | 0, ACDIRTY, ACUNDEF };
static unsigned short ac, acstate;
static void dca(int), lda(int), tad(int), and(int), jmp(int), cia(void), cma(void), sal(void);
static void reify(void);
static int writeback(void);

/* call frame management */
enum { FRAMESIZ = 00120, FRAMEORIG = 0060, VARSIZ = 00400 };
static struct expr savelabel = { "(save)", 0 }, framelabel = { "(frame)", 0 },
    stacklabel = { "(stack)", 0}, varlabel = { "(var)", 0 };
static unsigned short nframe, nvar, varspace, nstack, narg;
static signed short tos;
static unsigned short frame[FRAMESIZ], vars[VARSIZ];

static void newframe(const char *), emitvars(void);
static int spill(int);
static void pop(int);
static int push(int);

/* formatted assembly code output */
/* the tab stop in the output where we last wrote something */
enum { FLABEL, FINSTR, FOPERAND, FCOMMENT, FEOL };
static int field = -1;
static void advance(int);
static void label(struct expr *);
static void emit(const char *, ...);
static const char *lit(int);
static void comment(const char *, ...);
static void blank(void);
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
%right INC DEC '[' '('

%start	program

%%

program		: /* empty */
		| program definition
		;

definition	: define_name { comment(NAMEFMT, $1.name); } initializer ';'	/* simple definition */
		| define_name '[' vector_length ']' { emit(".+1"); comment(NAMEFMT, $1.name); } initializer ';' {
			/* vector definition */
			if ($3.value != EMPTY) {
				if ($3.value < $6.value)
					fprintf(stderr, NAMEFMT ": too many initializers\n", $1.name);

				if ($6.value < $3.value)
					emit("*.+%04o", $3.value - $6.value);
			}
		}
		| define_name { newframe($1.name); } '(' parameters ')' statement {
			/* function definition */
			reify(); /* DEBUG */
			emitvars();
		}
		;

define_name	: NAME {
			int i;

			blank();
			i = define(&$1);
			label(defns + i);
			$$ = defns[i];
		} ;

initializer	: /* empty */		{ $$.value = CONST | 0; }
		| ival_list
		;

ival_list	: ival			{ $$.value = CONST | 1; }
		| ival_list ',' ival	{ $$.value = $1.value + 1; }
		;

ival		: CONSTANT		{ emit("%s", lit($1.value)); }
		| NAME			{ define(&$1); emit("L%04o", $1.value & ~DSPMASK); }
		;

vector_length	: /* empty */		{ $$.value = EMPTY; }
		| CONSTANT
		;

parameters	: /* empty */
		| param_list
		;

param_list	: param
		| param_list ',' param
		;

param		: NAME {
			int i;

			$1.value = ARG | narg;
			i = declare(&$1);
			if (decls[i].value != $1.value)
				fprintf(stderr, NAMEFMT ": redeclared\n", $1.name);

			narg++;
		}
		;

statement	: AUTO auto_list ';' statement
		| EXTRN extrn_list ';' statement
		| NAME ':' statement
		| CASE CONSTANT ':' statement
		| DEFAULT ':' statement
		| '[' statement_list ']'
		| IF '(' expr ')' statement %prec ELSE
		| IF '(' expr ')' statement ELSE statement
		| WHILE '(' expr ')' statement
		| SWITCH '(' expr ')' statement		/* not original */
		| GOTO expr ';'
		| RETURN expr ';' {
			pop($2.value);
			writeback();
			lda($2.value);
			dca(RVALUE | 00056);
			jmp(LVALUE | 00057);
		}
		| RETURN ';' {
			writeback();
			jmp(LVALUE | 00057);
		}
		| BREAK ';'
		| expr ';'
		| ';'
		;

statement_list	: /* empty */
		| statement_list statement
		;

auto_list	: name_const
		| auto_list ',' name_const
		;

name_const	: NAME constant_opt {
			int i;

			$1.value = AUTOVAR | varspace;
			i = declare(&$1);
			if (decls[i].value != $1.value)
				fprintf(stderr, NAMEFMT ": redeclared\n", $1.name);
			else
				varspace++;
		}
		;

extrn_list	: extrn_decl
		| extrn_list ',' extrn_decl
		;

extrn_decl	: NAME {
			int i;

			define(&$1);
			i = declare(&$1);
			if ($1.value != decls[i].value)
				fprintf(stderr, NAMEFMT ": incompatible redeclaration\n", $1.name);
		}

constant_opt	: /* empty */ { vars[varspace] = CONST | 0; }
		| CONSTANT { vars[varspace] = $1.value; }
		;

arguments	: /* empty */
		| argument_list
		;

argument_list	: expr
		| argument_list ',' expr
		;

expr		: NAME {
			int i;


			for (i = ndecls - 1; i >= 0; i--)
				if (memcmp(decls[i].name, $1.name, MAXNAME) == 0)
					goto found;

			/* not found */
			fprintf(stderr, NAMEFMT ": undeclared\n", $1.name);
			exit(EXIT_FAILURE);

		found:	$$ = decls[i];
		}
		| CONSTANT
		| '(' expr ')' {
			$$ = $2;
		}
		| expr '(' arguments ')'
		| expr '[' expr ']'
		| INC expr
		| DEC expr
		| '+' expr %prec INC {
			$$.value = $2.value;
		}
		| '-' expr %prec INC {
			lda($2.value);
			pop($2.value);
			cia();
			$$.value = push(RSTACK);
		}
		| '*' expr %prec INC
		| '&' expr %prec INC
		| '^' expr %prec INC {
			lda($2.value);
			pop($2.value);
			cma();
			$$.value = push(RSTACK);
		}
		| expr INC
		| expr DEC
		| expr '*' expr
		| expr '%' expr
		| expr '/' expr
		| expr '+' expr {
			lda($3.value);
			pop($3.value);
			tad($1.value);
			pop($1.value);
			$$.value = push(RSTACK);
		}
		| expr '-' expr {
			lda($3.value);
			pop($3.value);
			cia();
			tad($1.value);
			pop($1.value);
			$$.value = push(RSTACK);
		}
		| expr SHL expr
		| expr SHR expr
		| expr '<' expr
		| expr '>' expr
		| expr LE expr
		| expr GE expr
		| expr EQ expr
		| expr NE expr
		| expr '&' expr {
			lda($3.value);
			pop($3.value);
			and($1.value);
			pop($1.value);
			$$.value = push(RSTACK);
		}
		| expr '^' expr {
			/* compute $1 + $3 - (($1 & $3) << 1) */
			lda($3.value);
			and($1.value);
			sal();
			cia();
			pop($3.value);
			tad($3.value);
			pop($1.value);
			tad($1.value);
			$$.value = push(RSTACK);
		}			
		| expr '\\' expr {
			/* compute $1 + $3 - ($1 & $3) */
			lda($3.value),
			and($1.value);
			cia();
			pop($3.value);
			tad($3.value);
			pop($1.value);
			tad($1.value);
			$$.value = push(RSTACK);
		}
		| expr '?' expr ':' expr
		| expr '=' expr {
			lda($3.value);
			pop($3.value);
			dca($1.value);
			$$.value = $1.value;
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

/*
 * write the content of AC to value and then clear AC.
 */
static void
dca(int value)
{
	reify();
	emit("DCA %s", lit(spill(value)));
	acstate = ac = CONST | 0;
}

/*
 * load AC with the given value.  
 */
static void
lda(int value)
{
	/* loads are idempotent */
	if (ac == value)
		return;

	writeback();
	ac = CONST | 0;
	tad(value);
	ac = value;
}

/*
 * Add b to AC.  The value of AC must be fixed up by the caller.
 * As a special case, adding a constant to a constant does not produce
 * code.
 */
static void
tad(int b)
{
	int a = ac;

	if (bothconst(a, b)) {
		ac = val(a + b) | CONST;
		return;
	}

	/* on writeback, reload */
	if (writeback()) {
		/* adding the constant first generates better code */
		if (dsp(b) == CONST) {
			int tmp = b;
			b = a;
			a = tmp;
		}

		lda(a);
	}

	reify();
	emit("TAD %s", lit(spill(b)));

	acstate = ACUNDEF;
	ac = UNDEF; /* caller must fix this */
}

/*
 * Compute the bitwise and of b and AC.  The value of AC must be fixed
 * up by the caller.
 */
static void
and(int b)
{
	int a = ac;

	if (bothconst(a, b)) {
		ac = val(a & b) | CONST;
		return;
	}

	/* on writeback, reload */
	if (writeback()) {
		/* placing the constant first generates better code */
		if (dsp(b) == CONST) {
			int tmp = b;
			b = a;
			a = tmp;
		}

		lda(a);
	}

	reify();
	emit("AND %s", lit(spill(b)));

	acstate = ACUNDEF;
	ac = UNDEF; /* caller must fix this */
}

/*
 * jump to the address of the argument. If the argument is in AC, also write
 * back AC first.
 */
static void
jmp(int addr)
{
	if (ac == addr && acstate == ACDIRTY)
		writeback();

	emit("JMP %s", lit(spill(addr)));
}

/*
 * compute the two's complement negation of AC (Complement and Increment Ac)
 */
static void
cia(void)
{
	int a = ac;

	if (dsp(a) == CONST) {
		ac = val(-a) | CONST;
		return;
	}

	reify();
	emit("CIA");
	acstate = ACUNDEF;
	ac = UNDEF; /* caller must fix this */
}

/*
 * compute the one's complement negation of AC (CoMplement Ac)
 */
static void
cma(void)
{
	int a = ac;

	if (dsp(a) == CONST) {
		ac = val(~a) | CONST;
		return;
	}

	reify();
	emit("CMA");
	acstate = ACUNDEF;
	ac = UNDEF; /* caller must fix this */
}

/*
 * add A to itself (Shift A Left)
 */
static void
sal(void)
{
	int a = ac;

	if (dsp(a) == CONST) {
		ac = val(a << 1) | CONST;
		return;
	}

	reify();
	emit("CLL RAL");
	acstate = ACUNDEF;
	ac = UNDEF; /* caller must fix this */
}

/*
 * Perform a writeback of AC if needed.  Return 1 if a writeback was
 * performed, 0 otherwise.  We don't call dca() here to avoid recursion.
 */
static int
writeback(void)
{
	if (dsp(ac) != CONST && acstate == ACDIRTY) {
		emit("DCA %s", lit(ac));
		ac = CONST | 0;
		acstate = ACCLEAR;
		return (1);
	} else
		return (0);
}

/*
 * make AC actually contain the value stored in ac and that writeback
 * has been performed.
 */
static void
reify(void)
{
	int acval = ac, loc;

	if (writeback())
		lda(acval);

	if (dsp(ac) != CONST)
		return;

	if (ac == acstate)
		return;

	switch (val(ac)) {
	case 00000:
		emit("CLA");
		break;

	case 00001:
		emit("CLA IAC");
		break;

	case 00002:
		emit("CLA CLL IAC RAL");
		break;

	case 00003:
		emit("CLA CLL CML IAC RAL");
		break;

	case 00004:
		emit("CLA CLL IAC RTL");
		break;

	case 00006:
		emit("CLA CLL CML IAC RTL");
		break;

	case 02000:
		emit("CLA CLL CML RTR");
		break;

	case 03777:
		emit("CLA CLL CMA RAR");
		break;

	case 04000:
		emit("CLA CLL CML RAR");
		break;

	case 05777:
		emit("CLA CLL CMA RTR");
		break;

	case 06000:
		emit("CLA CLL CML IAC RTL");
		break;

	case 07775:
		emit("CLA CLL CMA RTL");
		break;

	case 07776:
		emit("CLA CLL CMA RAL");
		break;

	case 07777:
		emit("CLA CMA");
		break;

	default:
		loc = spill(ac);
		lda(loc);
	}

	comment("CONST %04o", val(acval));
	acstate = acval;
}

/*
 * call frame management.
 *
 * The newframe() function initialises the variables used to control the
 * activation frame which is used to save volatile registers and keep
 * track of automatic variables.  Then it emits code to initialise the
 * call frame at runtime.  These variables are:
 *
 * nframe
 *     number of initialised registers in the call frame.  These
 *     registers contain constants and rvalues.  Their contents are
 *     loaded from the call frame on function entry and after
 *     returning from functions.  They should not be overwritten.
 *
 * narg
 *     the number of function arguments accounted for
 *
 * nvar
 *     number of automatic variables allocated.
 *
 * varspace
 *     number of words used for automatic variables.
 *
 * nstack
 *     number of stack registers used.  Stack registers are used to
 *     store temporary values.  They are saved during function calls
 *     but cannot be initialised.
 *
 * tos
 *     the first stack register that is in use.
 *
 * stacklabel
 *     a label referring to the first stack register.  This label is set
 *     to the value 0060 + nframe.
 *
 * savelabel
 *     a label referring to the beginning of the area where stack
 *     registers are saved during a function call.  This label only
 *     exists in leaf functions.
 *
 * varlabel
 *     a label referring to the beginning of the variable area in the
 *     call frame
 *
 * framelabel
 *     a label referring to the beginning of the call frame.
 */
static void
newframe(const char *name)
{
	nframe = 0;
	nvar = 0;
	varspace = 0;
	nstack = 0;
	tos = -1;
	ndecls = 0;
	narg = 0;

	framelabel.value = labelno++ | UNDEFN;
	stacklabel.value = labelno++ | UNDEFN;
	varlabel.value = labelno++ | UNDEFN;
	savelabel.value = EMPTY;

	emit("0"); /* return address */
	comment(NAMEFMT, name);
	emit("ECF");
	emit("%s", lit(framelabel.value));
	acstate = ac = CONST | 0;
}

/*
 * emit the call frame.  The call frame has the following
 * layout; ECF (establish call frame) is a routine called right at the
 * beginning of every function.
 *
 * nframe << 4 | narg
 *     the number of arguments and template registers used
 *
 * return_address
 *     the return address, copied here by ECF
 *
 * template[nframe]
 *     up to 80 template registers to be copied to addresses 0060--0177
 *     by ECF.
 *
 * arguments[narg]
 *     up to 16 function arguments copied here by ECF.
 *
 * variables[varspace]
 *     automatic variable data
 *
 * stackspace[nstack]
 *     space for saved registers during function calls
 */
static void
emitvars(void)
{
	size_t i;

	blank();

	/* beginning of stack */
	advance(FLABEL);
	printf("L%04o=\t%04o", stacklabel.value & ~DSPMASK, 0060 + nframe);
	field = FINSTR;
	comment("STACK ORIGIN");

	/* meta data */
	label(&framelabel);
	emit("%04o", nframe << 4 | narg);
	comment("CALL FRAME (%o REGS, %o ARGS)", nframe, narg);

	/* return address */
	emit("*.+1");
	comment("RETURN ADDRESS");

	/* arguments */
	if (narg > 0) {
		emit("*.+%o", narg);
		comment("FUNCTION ARGUMENTS");
	}

	/* template */
	for (i = 0; i < nframe; i++) {
		emit("%s", lit(frame[i]));
		comment("REGISTER %04o", FRAMEORIG + i);
	}

	/* automatic variables */
	if (varspace > 0) {
		blank();
		label(&varlabel);

		emit("%s", lit(vars[0]));
		comment("AUTOMATIC VARIABLES");

		for (i = 1; i < varspace; i++)
			emit("%s", lit(vars[i]));
	}

	/* stack save area; emit only if needed */
	if (savelabel.value != EMPTY) {
		blank();
		label(&savelabel);
		emit("%04o", nstack);
		emit("*.+%04o", nstack);
		comment("SAVED REGISTERS");
	}
}

/*
 * allocate a frame register with the value of spill.  Then return a
 * frame register that can be used as an argument to TAD, JMP, JMS, ISZ,
 * and DCA.  If the frame is full, return an error.  If a frame register
 * has already been allocated for this value, return that register.
 */
static int
spill(int value)
{
	int i, newdsp;

	switch (value & DSPMASK) {
	/* cases where no spill is needed */
	case LVALUE:
	case RVALUE:
	case LSTACK:
	case RSTACK:
		return (value);

	/* LVALUE cases */
	case LABEL:
	case UNDEFN:
	case AUTOVAR:
	case ARG:
		newdsp = LVALUE;
		break;

	/* RVALUE cases */
	default:
		fprintf(stderr, "cannot spill %06o\n", value);
		/* FALLTHROUGH */

	case CONST:
		newdsp = RVALUE;
		break;
	}

	/* let's see if we already have it */
	for (i = 0; i < nframe; i++)
		if (frame[i] == value)
			goto found;

	/* not found */
	frame[nframe] = value;
	if (nframe >= FRAMESIZ)
		fprintf(stderr, "frame overflown @ %04o by %06o\n", nframe, value);

	return (nframe++ + FRAMEORIG | newdsp);

found:	return (i + FRAMEORIG | newdsp);
}

/*
 * Remove value from the stack.  If value is not on the stack, do
 * nothing.  Else if val(value) != tos - 1, signal a programming error.
 */
static void
pop(int value)
{
	if (!onstack(value))
		return;

	if (val(value) != tos)
		fprintf(stderr, "cannot pop %06o\n", value);
	else
		tos--;

	if (ac == value && acstate == ACDIRTY)
		acstate = ACUNDEF;
}

/*
 * if AC holds a constant, return that constant.  If not, allocate a new
 * stack register and indicate that AC needs to be written back to it.
 * Use disp for its disposition.  disp should be RSTACK or LSTACK.
 */
static int
push(int disp)
{
	if (dsp(ac) == CONST)
		return (ac);

	acstate = ACDIRTY;
	ac = ++tos | disp;

	return (ac);
}

/*
 * Advance output to field f.  If we are already past this field, begin
 * a new line.
 */
static void
advance(int f)
{
	int i;

	if (field >= f) {
		putchar('\n');
		field = 0;
	} else if (field == -1)
		field = 0;

	for (i = field; i < f; i++)
		putchar('\t');

	field = f;
}

/*
 * If expr is EMPTY, generate a new label named L#### for some octal
 * number #### and emit it.  The number of the next label to use is
 * taken from labelno.  Otherwise, if expr is UNDEFN, use the label
 * number from expr.value.  Otherwise, indicate a redefined label.
 * Update expr to type LABEL with the label number used.
 */
static void
label(struct expr *expr)
{
	int no;

	advance(FLABEL);

	switch (expr->value & DSPMASK) {
	case LABEL:
		fprintf(stderr, NAMEFMT ": redefined\n", expr->name);
		/* FALLTHROUGH */

	case EMPTY:
		no = labelno++;
		break;

	case UNDEFN:
		no = expr->value & ~DSPMASK;
		break;

	default:
		fprintf(stderr, NAMEFMT ": unexpected value %05o\n", expr->name, expr->value);
		abort();
	}

	printf("L%04o,", no);

	expr->value = no | LABEL;
}

/*
 * emit an instruction into the instruction field.  printf-like
 * syntax can be used.
 */
static void
emit(const char *fmt, ...)
{
	int n;

	va_list ap;

	advance(FINSTR);

	va_start(ap, fmt);
	n = vprintf(fmt, ap);
	va_end(ap);

	if (n >= 16)
		field = FCOMMENT;
	else if (n >= 8)
		field = FOPERAND;

}

/*
 * return the a string representation of the location lit is in.
 * The result is placed in a static buffer that is overvwritten
 * by successive calls.
 */
static const char *lit(int v)
{
	static char buf[17];

	switch(v & DSPMASK) {
	case LABEL:
	case UNDEFN:
		sprintf(buf, "L%04o", v & ~DSPMASK);
		break;

	case LVALUE:
		sprintf(buf, "I %04o", v & ~DSPMASK);
		break;

	case LSTACK:
		sprintf(buf, "I L%04o+%04o", stacklabel.value & ~DSPMASK,
		    v & ~DSPMASK);
		break;

	case RSTACK:
		sprintf(buf, "L%04o+%04o", stacklabel.value & ~DSPMASK,
		    v & ~DSPMASK);
		break;

	case AUTOVAR:
		sprintf(buf, "L%04o+%04o", varlabel.value & ~DSPMASK,
		    v & ~DSPMASK);
		break;

	case ARG:
		sprintf(buf, "L%04o+%04o", framelabel.value & ~DSPMASK,
		    (v & ~DSPMASK) + 2);
		break;

	case RVALUE:
	case CONST:
		sprintf(buf, "%04o", v & ~DSPMASK);
		break;

	default:
		/* cause a syntax error */
		sprintf(buf, "??? %06o", v);
	}

	return (buf);
}

/*
 * Print a comment at the end of the current line using printf-like
 * syntax.
 */
static void
comment(const char *fmt, ...)
{
	va_list ap;

	advance(FCOMMENT);

	printf("/ ");
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

/*
 * emit a blank line unless the previous line was already a blank line.
 */
static void
blank(void)
{
	if (field != -1) {
		putchar('\n');
		putchar('\n');
		field = -1;
	}
}

/*
 * the main function.  Start the parser and then terminate the output
 * with a dollar sign.
 */
extern int
main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	yyparse();
	advance(FINSTR);
	printf("$\n");

	return (EXIT_SUCCESS);
}
