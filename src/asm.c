/* asm.c -- PAL pretty printer */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "param.h"
#include "asm.h"
#include "error.h"
#include "pdp8.h"

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
 * The variable isskip indicates that the next instruction is
 * conditionally executed and is thus printed prefixed with a blank.
 * This variable is set by skip() and cleared after every emitted
 * instruction.
 */
static char isskip = 0;

/*
 * advance to the indicated field by emitting tabulators.  If we are
 * already past that column, start a new line.
 */
static void
field(int target)
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

	field(FLABEL);

	va_start(ap, fmt);
	column += vfprintf(asmfile, fmt, ap);
	va_end(ap);
}

extern void
instr(const char *fmt, ...)
{
	va_list ap;

	field(FINSTR);

	if (isskip)
		fputc(' ', asmfile);

	va_start(ap, fmt);
	column += vfprintf(asmfile, fmt, ap) + isskip;
	va_end(ap);

	isskip = 0;
}

extern void
comment(const char *fmt, ...)
{
	va_list ap;

	field(FCOMMENT);

	fputc('/', asmfile);
	fputc(' ', asmfile);

	va_start(ap, fmt);
	column += vfprintf(asmfile, fmt, ap) + 2;
	va_end(ap);
}

extern void
commentname(const char *name)
{
	if (name[0] != '\0')
		comment(NAMEFMT, name);
}

extern void
endline(void)
{
	field(FBEGIN);
}

extern void
blank(void)
{
	/* do not output multiple blank lines in succession */
	if (column == 0)
		return;

	field(FBEGIN);
	fputc('\n', asmfile);
}

extern void
emitc(int c)
{
	instr("%04o", c & 07777);
}

extern void
advance(int n)
{
	if (n > 0)
		instr("*.+%04o", n);
}

extern void
skip(void)
{
	isskip = 1;
}
