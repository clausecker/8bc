%{
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

definition	: define_name initializer ';' /* simple definition */
		| define_name '[' {
			instr(".+1"); /* TODO: perhaps add this to pdp8.h */
			comment(NAMEFMT, $1.name);
		} vector_length ']' initializer ';' { /* vector definition */
			int want, have;

			want = val($4.value);
			have = val($6.value);

			if (have > want)
				want = have;

			if (want == 0)
				warn($1.name, "zero length vector");

			skip(want);
		}
		| define_name '(' parameters ')' statement /* function definition */
		;

define_name	: NAME {
			struct expr *e;

			e = define($1.name);
			if (rclass(e->value) != RUND)
				error($1.name, "redefined");
			else
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

param		: NAME
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
		| expr ';'
		| ';'
		;

label		: NAME
		| CASE CONSTANT
		| DEFAULT
		;

statement_list	: /* empty */
		| statement_list statement
		;

auto_list	: name_const
		| auto_list ',' name_const
		;

name_const	: NAME constant_opt
		;

extrn_list	: extrn_decl
		| extrn_list ',' extrn_decl
		;

extrn_decl	: NAME
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
		| expr INC
		| expr DEC
		| INC expr
		| DEC expr
		| '+' expr %prec INC
		| '-' expr %prec INC
		| '*' expr %prec INC
		| '&' expr %prec INC
		| '^' expr %prec INC
		| '!' expr
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
