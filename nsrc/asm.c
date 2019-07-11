/* asm.c -- PAL pretty printer */

#include <stdarg.h>
#include <stdio.h>

#include "asm.h"

FILE *asmfile = NULL;

/*
 * column tracks where we currently are on the line, with 0 being the
 * first column on the line.  Each field starts at the column specified
 * in the enumeration below.  Tabulators are used to advance to the next
 * field.
 */
static unsigned short column = 0;
enum {
	FBEGIN = 0,		/* beginning of line */
	FLABEL = FBEGIN,	/* label field */
	FINSTR = 8,		/* instruction field */
	FCOMMENT = 24,		/* commment field */
};

/*
 * advance to the indicated column by emitting tabulators.  If we are
 * already past that column, start a new line.
 */
static void
advance(int target)
{
	if (column > target || column == target && target > 0) {
		fputc('\n', asmfile);
		column = 0;
	}

	/* align to a multiple of 8 */
	column &= ~7;

	while (column < target) {
		fputc('\t', asmfile);
		column += 8;
	}
}

extern void
label(const char *fmt, ...)
{
	va_list ap;

	advance(FLABEL);

	va_start(ap, fmt);
	column += vprintf(fmt, ap);
	va_end(ap);
}

extern void
instr(const char *fmt, ...)
{
	va_list ap;

	advance(FINSTR);

	va_start(ap, fmt);
	column += vprintf(fmt, ap);
	va_end(ap);
}

extern void
comment(const char *fmt, ...)
{
	va_list ap;

	advance(FCOMMENT);

	fputc('/', asmfile);
	fputc(' ', asmfile);

	va_start(ap, fmt);
	column += vprintf(fmt, ap) + 2;
	va_end(ap);
}

extern void
endline(void)
{
	advance(FBEGIN);
}

extern void
blank(void)
{
	/* do not output multiple blank lines in succession */
	if (column == 0)
		return;

	advance(FBEGIN);
	fputc('\n', asmfile);
}
