%{
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "name.h"

extern int yyerror(const char *);
extern int yylex(void);

/* the tab stop in the output where we last wrote something */
enum {
	FLABEL,
	FINSTR,
	FOPERAND,
	FCOMMENT,
	FEOL,
};

int field = -1;

static void advance(int);
static void label(struct expr *);
static void emit(const char *, ...);
static void comment(const char *, ...);

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
%right INC DEC
%right '[' '('

%start	program

%%

program		: /* empty */
		| program definition
		;

definition	: define_name initializer ';'			/* simple definition */
		| define_name '[' vector_length ']' { emit(".+1"); } initializer ';' {
			/* vector definition */
			if ($3.value != EMPTY) {
				if ($3.value < $6.value)
					fprintf(stderr, NAMEFMT ": too many initializers\n", $1.name);

				if ($6.value < $3.value)
					emit("*.+%04o", $3.value - $6.value);
			}
		}
		| define_name '(' parameters ')' statement	/* function definition */
		;

define_name	: NAME {
			int i = define(&$1);
			label(defns + i);
			comment(NAMEFMT, $1.name);
			$$ = defns[i];
		} ;

initializer	: /* empty */		{ $$.value = CONST | 0; }
		| ival_list
		;

ival_list	: ival			{ $$.value = CONST | 1; }
		| ival_list ',' ival	{ $$.value = $1.value + 1; }
		;

ival		: CONSTANT		{ emit("%04o", $1.value & ~DSPMASK); }
		| NAME			{ define(&$1); emit("L%04o", $1.value & ~DSPMASK); }
		;

vector_length	: /* empty */		{ $$.value = EMPTY; }
		| CONSTANT
		;

parameters	: /* empty */
		| name_list
		;

name_list	: NAME
		| name_list ',' NAME
		;

statement	: AUTO name_const_list ';' statement
		| EXTRN name_list ';' statement
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
		| BREAK ';'
		| expr ';'
		| ';'
		;

statement_list	: /* empty */
		| statement_list statement
		;

name_const_list	: NAME constant_opt
		| name_const_list ',' NAME constant_opt
		;

constant_opt	: /* empty */
		| CONSTANT
		;

arguments	: /* empty */
		| argument_list
		;

argument_list	: expr
		| argument_list ',' expr
		;

expr		: NAME
		| CONSTANT
		| '(' expr ')'
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
static void emit(const char *fmt, ...)
{
	va_list ap;

	advance(FINSTR);

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

/*
 * Print a comment at the end of the current line using printf-like
 * syntax.
 */
static void comment(const char *fmt, ...)
{
	va_list ap;

	advance(FCOMMENT);

	printf("/ ");
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
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
