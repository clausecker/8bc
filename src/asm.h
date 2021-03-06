/*- (c) 2019 Robert Clausecker <fuz@fuz.su> */
/* asm.h -- PAL pretty printer */

/*
 * This module deals with generating formatted assembly output.
 *
 * Assembly code is generated in three columns:
 *
 *  - label (1 tab stop) contains labels for the current line.
 *  - instruction (2 tab stops) contains instructions and their
 *    operands
 *  - comment (rest of the line) contains comments.
 *
 * Text can be written to each of the three columns with the
 * functions label(), instr(), and comment().  In the case of comment(),
 * the comment character and a space is implicitly preprended.  A blank
 * line can be placed in the source file with blank().  Finally, the
 * current line can be finished with endline().  Assembly output is
 * written to asmfile which must be set on program startup.  No error
 * checking is done; it is advised to call ferror(asmfile) at the end to
 * check for IO errors.
 */
extern FILE *asmfile;
extern void label(const char *fmt, ...);
extern void instr(const char *fmt, ...);
extern void comment(const char *fmt, ...);
extern void blank(void), endline(void);

/*
 * High-level functions.
 *
 * emitc(c)
 *     Emit the octal representation of c.
 *
 * advance(n)
 *     If n is nonzero, emit a request to advance by n words.
 *
 * skip()
 *     Indicate that the next instruction is conditionally executed,
 *     causing it to be printed with a leading blank.  This is
 *     automatically performed when a skipping OPR instruction is
 *     emitted.
 *
 * commentname(name)
 *     If name is not an empty string, print a comment for name using
 *     NAMEFMT.
 */
extern void emitc(int);
extern void advance(int);
extern void skip(void);
extern void commentname(const char *);
