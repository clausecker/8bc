%{
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "name.h"

extern int yyerror(const char *);
extern int yylex(void);

/* AC manipulation */
static unsigned short ac = EMPTY;
static void dca(int), lda(int), tad(int), spill(void);

/* call frame management */
static struct expr savelabel = { "(save)", 0 }, framelabel = { "(frame)", 0 },
    stacklabel = { "(stack)", 0}, varlabel = { "(var)", 0 };
static unsigned short nframe, nvar, varspace, nstack, tos, narg;
static unsigned short frame[80], vars[256];

static void newframe(const char *), emitvars(void);
static int newvar(void);
static void vardata(int);
static int stalloc(void);
static void stfree(int);

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
		| RETURN expr ';'
		| RETURN ';'
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
		| '(' expr ')'			{ $$ = $2; }
		| expr '(' arguments ')'
		| expr '[' expr ']'
		| INC expr
		| DEC expr
		| '+' expr
		| '-' expr
		| '*' expr
		| '&' expr
		| '^' expr
		| expr INC
		| expr DEC
		| expr '*' expr
		| expr '%' expr
		| expr '/' expr
		| expr '+' expr
		| expr '-' expr
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
		| expr '=' expr
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
 *     the first stack register that is currently free.
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
	tos = 0;
	ndecls = 0;

	framelabel.value = labelno++ | UNDEFN;
	stacklabel.value = labelno++ | UNDEFN;
	varlabel.value = labelno++ | UNDEFN;
	savelabel.value = EMPTY;

	emit("0"); /* return address */
	comment(NAMEFMT, name);
	emit("ECF");
	emit("%s", lit(framelabel.value));
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
	if (nframe > 0) {
		emit("%s", lit(frame[0]));
		comment("REGISTER TEMPLATE");

		for (i = 1; i < nframe; i++)
			emit("%s", lit(frame[i]));
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
	}
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
	if (n >= 8)
		field = FOPERAND;

	va_end(ap);
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
