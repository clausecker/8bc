/*- (c) 2019 Robert Clausecker <fuz@fuz.su> */
/* data.h -- data area management */

/*
 * The compiler stores string literals and numerical constants in a
 * constant data area.  This data area is manipulated with the
 * following functions:
 *
 * todata(int c)
 *     append word c to the constant area.
 *
 * newdata(expr)
 *     make expr->value refer to the current location in the data area.
 *
 * literal(expr, c)
 *     find and occurence of c in the data area and make expr->value
 *     point to it.  If such an occurence does not exist, append c to
 *     the data area to create one.  c should be of type RCONST, RLABEL,
 *     RDATA, RAUTO, or RPARAM.
 *
 * dumpdata()
 *     dump the content of the data area into the assembly output.
 */
extern void todata(int);
extern void newdata(struct expr *);
extern void literal(struct expr *, const struct expr *);
extern void dumpdata(void);
