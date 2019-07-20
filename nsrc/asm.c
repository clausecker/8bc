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
 * Generate a group 1 microcoded instruction.  Up to four instructions
 * may be emitted:
 *
 * one of CLA CMA STA
 * one of CLL CML bSTL
 * one of RAR RAL BSW RTR RTL
 * finally, IAC
 *
 * It is assumed that op refers to a group 1 microcoded instruction.
 * Return 1 on success, 0 if op has an unsupported combination of bits
 * set.
 */
static int
opr1(char *buf, int op)
{
	const char *suffix;
	static const char rots[6][4] = { "", "BSW ", "RAL ", "RTL ", "RAR ", "RTR " };

	switch (op & (CLA | CMA)) {
	case CLA: suffix = "CLA "; break;
	case CMA: suffix = "CMA "; break;
	case STA: suffix = "STA "; break;
	default:  suffix = "";
	}

	strcpy(buf, suffix);

	switch (op & (CLL | CML)) {
	case CLL: suffix = "CLL "; break;
	case CML: suffix = "CML "; break;
	case STL: suffix = "STL "; break;
	default:  suffix = "";
	}

	strcat(buf, suffix);

	/* can't have RAL and RAR set at the same time */
	if ((op & (RAL | RAR)) == (RAL | RAR))
		return (0);

	/* inspect RAL, RAR, and BSW */
	strncat(buf, rots[op >> 1 & 7], 4);

	if ((op & IAC) == IAC)
		strcat(buf, "IAC ");

	return (1);
}

/*
 * Generate a group 2 microcoded instruction.  Up to four instructions
 * may be generated:
 *
 *    any of SMA SZA SNL
 * or any of SPA SNA SZL
 * and a CLA.
 *
 * It is assumed that op refers to a group 2 microcoded instruction.
 * Return 1 on success, 0 if op has an unsupported combination of bits
 * set.
 */
static int
opr2(char *buf, int op)
{
	static const char mnemo[2][3][4] = { "SNL ", "SZA ", "SMA ", "SZL ", "SNA ", "SPA " };
	int i, skip;

	skip = (op & SKP) == SKP;

	for (i = 0; i < 3; i++)
		if (op & 00020 << i)
			strncat(buf, mnemo[skip][i], 4);

	if ((op & CLA) == CLA)
		strcat(buf, "CLA ");

	return (1);
}

extern void
emitopr(int op)
{
	char buf[4 * 4 + 1];
	int succeeded = 1;

	op &= 07777;
	buf[0] = '\0';

	if ((op & OPR1) != OPR1)
		succeeded = 0;  /* not an OPR instruction */
	else if ((op & 00400) == 0)
		succeeded = opr1(buf, op);   /* OPR group 1 */
	else if ((op & 00007) == 0)
		succeeded = opr2(buf, op);   /* OPR group 2 */
	else
		succeeded = 0;  /* OPR group 3 or privileged */

	if (succeeded) {
		if (buf[0] == '\0')
			strcpy(buf, "NOP");
		else	/* trim trailing whitespace */
			buf[strlen(buf) - 1] = '\0';
	} else {
		sprintf(buf, "%04o", op);
		warn(buf, "invalid OPR instruction");
	}

	instr(buf);
}
