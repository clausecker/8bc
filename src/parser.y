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

%start	program

%%

program	: /* empty */
	| program definition
	;

definition
	: NAME initializer ';'				/* simple definition */
	| NAME '[' vector_length ']' initializer ';'	/* vector definition */
	| NAME '(' parameters ')' statement		/* function definition */
	;

initializer
	: /* empty */
	| ival ival_list
	;

ival_list
	: /* empty */
	| ival_list ',' ival
	;

ival	: CONSTANT
	| NAME
	;

vector_length
	: /* empty */
	| CONSTANT
	;

parameters
	: /* empty */
	| name_list
	;

name_list
	: NAME
	| name_list ',' NAME
	;

statement
	: AUTO name_const_list ';' statement
	| EXTRN name_list ';' statement
	| NAME ':' statement
	| CASE CONSTANT ':' statement
	| DEFAULT ':' statement
	| '[' statement_list ']'
	| IF '(' control ')' statement else_clause
	| WHILE '(' control ')' statement
	| SWITCH '(' rvalue ')' statement		/* not original */
	| GOTO rvalue ';'
	| RETURN rvalue ';'
	| BREAK ';'
	| rvalue ';'
	| ';'

statement_list
	:
	| statement_list statement
	;

name_const_list
	: NAME constant_opt
	| name_const_list ',' NAME constant_opt
	;

constant_opt
	: /* empty */
	| CONSTANT
	;

else_clause
	: /* empty */
	| ELSE statement
	;

arguments
	: /* empty */
	| argument_list
	;

argument_list
	: rvalue
	| argument_list ',' rvalue
	;

assign	: '='
	| ASOR
	| ASXOR
	| ASAND
	| ASEQ
	| ASNE
	| ASLT
	| ASLE
	| ASGT
	| ASGE
	| ASSHL
	| ASSHR
	| ASADD
	| ASSUB
	| ASMOD
	| ASMUL
	| ASDIV
	;

inc_dec	: INC
	| DEC
	;

primary_rvalue
	: CONSTANT
	| '(' rvalue ')'
	;

postfix_rvalue
	: primary_rvalue
	| postfix_lvalue
	| postfix_rvalue '(' arguments ')'
	| postfix_lvalue inc_dec
	;

postfix_lvalue
	: NAME
	| postfix_rvalue '[' rvalue ']'
	;

unary_rvalue
	: postfix_rvalue
	| lvalue
	| '+' unary_rvalue
	| '-' unary_rvalue
	| inc_dec lvalue
	| '&' lvalue
	;

lvalue	: postfix_lvalue
	| '*' postfix_rvalue
	;

multiplicative_rvalue
	: unary_rvalue
	| multiplicative_rvalue '*' unary_rvalue
	| multiplicative_rvalue '/' unary_rvalue
	| multiplicative_rvalue '%' unary_rvalue
	;

additive_rvalue
	: multiplicative_rvalue
	| additive_rvalue '+' multiplicative_rvalue
	| additive_rvalue '-' multiplicative_rvalue
	;

shift_rvalue
	: additive_rvalue
	| shift_rvalue SHL additive_rvalue
	| shift_rvalue SHR additive_rvalue
	;

	/* TODO: find out if relational operators have precedence */
relational_rvalue
	: shift_rvalue
	| shift_rvalue '<' shift_rvalue
	| shift_rvalue '>' shift_rvalue
	| shift_rvalue LE shift_rvalue
	| shift_rvalue GE shift_rvalue
	;

	/* TODO: find out if equality operators have precedence */
equality_rvalue
	: relational_rvalue
	| relational_rvalue EQ relational_rvalue
	| relational_rvalue NE relational_rvalue
	;

	/* bitwise and */
and_rvalue
	: equality_rvalue
	| and_rvalue '&' equality_rvalue
	;

	/* logical and */
and_control
	: equality_rvalue
	| and_control '&' equality_rvalue
	;

	/* bitwise xor */
xor_rvalue
	: and_rvalue
	| xor_rvalue '^' and_rvalue
	;

	/* logical xor */
xor_control
	: and_control
	| xor_control '^' and_control
	;

	/* birwise or */
or_rvalue
	: xor_rvalue
	| or_rvalue '\\' xor_rvalue
	;

	/* logical or */
or_control
	: xor_control
	| or_control '\\' xor_control
	;

conditional_rvalue
	: or_rvalue
	| or_control '?' rvalue ':' conditional_rvalue
	;

conditional_control
	: or_control
	| or_control '?' rvalue ':' conditional_rvalue
	;

rvalue	: conditional_rvalue
	| lvalue assign rvalue
	;

control	: conditional_control
	| lvalue assign rvalue
	;
