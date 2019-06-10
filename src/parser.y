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

%nonassoc IFTHEN
%nonassoc PURE
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
%right '[' '(' POSTFIX
%left INC DEC UNARY

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
	| IF '(' rvalue ')' statement %prec IFTHEN
	| IF '(' rvalue ')' ELSE statement
	| WHILE '(' rvalue ')' statement
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

arguments
	: /* empty */
	| argument_list
	;

argument_list
	: rvalue
	| argument_list ',' rvalue
	;

rvalue	: '(' rvalue ')'
	| CONSTANT
	| INC lvalue %prec UNARY
	| DEC lvalue %prec UNARY
	| '+' rvalue %prec UNARY
	| '-' rvalue %prec UNARY
	| '&' lvalue %prec UNARY
	| lvalue INC %prec POSTFIX
	| lvalue DEC %prec POSTFIX
	| rvalue '*' rvalue
	| rvalue '%' rvalue
	| rvalue '/' rvalue
	| rvalue '+' rvalue
	| rvalue '-' rvalue
	| rvalue SHL rvalue
	| rvalue SHR rvalue
	| rvalue '<' rvalue
	| rvalue '>' rvalue
	| rvalue LE rvalue
	| rvalue GE rvalue
	| rvalue EQ rvalue
	| rvalue NE rvalue
	| rvalue '&' rvalue
	| rvalue '^' rvalue
	| rvalue '\\' rvalue
	| rvalue '?' rvalue ':' rvalue
	| lvalue '=' rvalue
	| lvalue ASMUL rvalue
	| lvalue ASMOD rvalue
	| lvalue ASDIV rvalue
	| lvalue ASADD rvalue
	| lvalue ASSUB rvalue
	| lvalue ASSHL rvalue
	| lvalue ASSHR rvalue
	| lvalue ASLT rvalue
	| lvalue ASGT rvalue
	| lvalue ASLE rvalue
	| lvalue ASGE rvalue
	| lvalue ASEQ rvalue
	| lvalue ASNE rvalue
	| lvalue ASAND rvalue
	| lvalue ASXOR rvalue
	| lvalue ASOR rvalue
	| rvalue '(' arguments ')'
	| lvalue %prec PURE
	;

lvalue	: NAME
	| '*' rvalue %prec UNARY
	| rvalue '[' rvalue ']'
	;
