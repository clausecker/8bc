/*
 * Error messages are printed with the error function.  If instead the
 * fatal function is called, the program is terminated immediately.
 * Emitting any error increases the errcnt.  If more than MAXERROR
 * errors are emitted, the program is terminated as with fatal.  The
 * warn function behaves like error but does not change errcnt.  Error
 * messages have the form
 *
 *     ##### XXXXXXXX ...
 *
 * where ##### is the current line number, XXXXXXXX is the name passed
 * to error and ... is the error message passed to error.  If name is
 * NULL, XXXXXXXX is omitted.  A line break is appended to the error
 * message.
 */
extern short errcnt;
extern void warn(const char *name, const char *fmt, ...);
extern void error(const char *name, const char *fmt, ...);
extern void fatal(const char *name, const char *fmt, ...);

/*
 * compatibility function to generate lexer and parser errors.
 */
extern void yyerror(const char *);
