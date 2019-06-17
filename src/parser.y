%{
#include "name.h"

extern int yyerror(const char *);
extern int yylex(void);
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

definition	: NAME initializer ';'				/* simple definition */
		| NAME '[' vector_length ']' initializer ';'	/* vector definition */
		| NAME '(' parameters ')' statement		/* function definition */
		;

initializer	: /* empty */
		| ival ival_list
		;

ival_list	: /* empty */
		| ival_list ',' ival
		;

ival		: CONSTANT
		| NAME
		;

vector_length	: /* empty */
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
