/*- (c) 2019 Robert Clausecker <fuz@fuz.su> */
/* parser.h -- definitions for lexer.l and parser.y */

#define YYSTYPE struct expr
extern YYSTYPE yylval;

extern int yylex(void);
extern int yyparse(void);

/*
 * The current line number in the input file.  Used to generate error
 * messages.  The first line is line number 1.
 */
extern unsigned short lineno;
