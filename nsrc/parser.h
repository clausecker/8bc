/* definitions for lexer.l and parser.y */

#define YYSTYPE struct expr
extern YYSTYPE yylval;

extern int yylex(void);
extern int yyparse(void);
