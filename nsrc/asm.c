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
commentname(const char *name)
{
	if (name[0] != '\0')
		comment(NAMEFMT, name);
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

extern void
emitc(int c)
{
	instr("%04o", c & 07777);
}

extern void
skip(int n)
{
	if (n > 0)
		instr("*.+%04o", n);
}

/*
 * Emit a group 1 microcoded instruction.  Up to four instructions may
 * be emitted:
 *
 * one of CLA CMA STA
 * one of CLL CML bSTL
 * one of RAR RAL BSW RTR RTL
 * finally, IAC
 *
 * if no bit is set, a NOP is emitted.  It is assumed that op refers to
 * a group 1 microcoded instruction.
 */
static void
opr1(int op)
{
	static char buf[4 * 4 + 1];

	buf[0] = '\0';

	switch (op & (CLA | CMA)) {
	case CLA: strcpy(buf, " CLA"); break;
	case CMA: strcpy(buf, " CMA"); break;
	case STA: strcpy(buf, " STA"); break;
	}

	switch (op & (CLL | CML)) {
	case CLL: strcat(buf, " CLL"); break;
	case CML: strcat(buf, " CML"); break;
	case STL: strcat(buf, " STL"); break;
	}

	switch (op & (RAR | RAL | BSW)) {
	case OPR1: break;
	case RAR: strcat(buf, " RAR"); break;
	case RAL: strcat(buf, " RAL"); break;
	case BSW: strcat(buf, " BSW"); break;
	case RTR: strcat(buf, " RTR"); break;
	case RTL: strcat(buf, " RTL"); break;

	default:
		fatal(NULL, "invalid OPR instruction %04o", op);
	}

	if ((op & IAC) == IAC)
		strcat(buf, " IAC");

	instr(buf[0] == '\0' ? "NOP" : buf + 1);
}

/*
 * Emit a group 2 microcoded instruction.  Up to four instructions may
 * be emitted:
 *
 *       any of SMA SZA SNL
 * or up any of SPA SNA SZL
 * and a CLA.
 *
 * If none of the bits are set, a NOP is emitted.  It is assumed that
 * op refers to a group 2 microcoded instruction.
 */
static void
opr2(int op)
{
	static const char mnemo[2][3][5] = { " SNL", " SZA", " SMA", " SZL", " SNA", " SPA" };
	static char buf[4 * 4 + 1];
	int i, skip;

	buf[0] = '\0';
	skip = (op & SKP) == SKP;

	for (i = 0; i < 3; i++)
		if (op & 00020 << i)
			strcat(buf, mnemo[skip][i]);

	if ((op & CLA) == CLA)
		strcat(buf, " CLA");

	instr(buf[0] == '\0' ? "NOP" : buf + 1);
}

extern void
emitopr(int op)
{
	if ((op & OPR1) != OPR1)
		goto inval; /* not an OPR instruction */
	else if ((op & 00400) == 0)
		opr1(op);   /* OPR group 1 */
	else if ((op & 00007) == 0)
		opr2(op);   /* OPR group 2 */
	else
		goto inval; /* OPR group 3 or privileged */

	return;

inval:	fatal(NULL, "invalid arg to %s: %06o", __func__, op);
}
