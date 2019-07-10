/* pdp8.c -- PDP-8 code generation */

#include <stdio.h>
#include <string.h>

#include "param.h"
#include "asm.h"
#include "error.h"
#include "pdp8.h"
#include "name.h"

/*
 * Labels related to the current function.
 *
 * framelabel
 *     beginning of the frame record
 *
 * stacklabel
 *     first register on the stack
 *
 * autolabel
 *     beginning of the automatic variable area
 */
static struct expr framelabel = { 0, "(frame)" };
static struct expr stacklabel = { 0, "(stack)" };
static struct expr autolabel = { 0, "(auto)" };

/*
 * Generate a string representation of the address of e as needed for
 * emitl.  e must be of type LCONST, RVALUE, LLABEL, LUND, RSTACK,
 * RAUTO, or RARG.  The returned string is placed in a static buffer.
 */
static const char *
lstr(const struct expr *e)
{
	static char buf[14];
	int v = e->value;

	switch (class(v)) {
	case LCONST:
	case RVALUE:
		sprintf(buf, "%04o", val(v));
		break;

	case LLABEL:
	case LUND:
		sprintf(buf, "L%04o", val(v));
		break;

	case RSTACK:
		sprintf(buf, "L%04o+%03o", val(stacklabel.value), val(v));
		break;

	case RAUTO:
		sprintf(buf, "L%04o+%03o", val(autolabel.value), val(v));
		break;

	case RARG:
		sprintf(buf, "L%04o+%03o", val(framelabel.value), val(v) + 2);
		break;

	default:
		fatal(e->name, "invalid arg to %s: %06o", __func__, val(v));
	}

	return (buf);
}

extern void
emitl(const struct expr *e)
{
	return (instr(lstr(e)));
}

extern void
emitr(const struct expr *e)
{
	struct expr le;

	switch (class(e->value)) {
	case RCONST:
	case RLABEL:
	case RUND:
		le = r2lval(e);

		return (instr(lstr(&le)));

	default:
		fatal(e->name, "invalid arg to %s: %06o", __func__, val(e->value));
	}
}

/*
 * Turn expr into a string suitable as an argument to a PDP-8
 * instruction.  If needed, spill the argument into a zero page
 * register.  The resulting string is stored in a static buffer
 * which is reused by the next call to this function.
 */
static const char *
arg(const struct expr *e)
{
	static char buf[16];
	int v = e->value;
	const char *prefix = "";

	switch (class(v)) {
	case LLABEL:
	case LUND:
	case RLABEL:
	case RUND:
	case LCONST:
	case RCONST:
		/* TODO: spill */
		fatal(e->name, "spill not implemented");

	case LVALUE:
	case LSTACK:
	case LAUTO:
	case LARG:
		prefix = "I ";
		break;

	case RVALUE:
	case RSTACK:
	case RAUTO:
	case RARG:
		break;

	default:
		fatal(e->name, "invalid arg to %s: %06o", __func__, val(v));
	}

	sprintf(buf, "%s%s", prefix, lstr(e));

	return (buf);
}

extern struct expr
r2lval(const struct expr *e)
{
	struct expr r = *e;
	int v = e->value;

	if (isvalid(v) && isrval(v))
		r.value = v | LMASK;
	else
		r.value = NORVAL;

	return (r);
}

extern struct expr
l2rval(const struct expr *e)
{
	struct expr r = *e;
	int v = e->value;

	if (isvalid(v) && islval(v))
		r.value = v & ~LMASK;
	else
		r.value = NOLVAL;

	return (r);
}

extern void
and(const struct expr *e)
{
	instr("AND %s", arg(e));
}

extern void
tad(const struct expr *e)
{
	instr("TAD %s", arg(e));
}

extern void
isz(const struct expr *e)
{
	instr("ISZ %s", arg(e));
}

extern void
dca(const struct expr *e)
{
	instr("DCA %s", arg(e));
}

extern void
jms(const struct expr *e)
{
	instr("JMS %s", arg(e));
}

extern void
jmp(const struct expr *e)
{
	instr("JMP %s", arg(e));
}

extern void
lda(const struct expr *e)
{
	opr(CLA);
	tad(e);
}

/*
 * Emit a group 1 microcoded instruction.  Up to four instructions may
 * be emitted:
 *
 * one of CLA CMA STA
 * one of CLL CML STL
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
	case 0: break;
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
opr(int op)
{
	if ((op & OPR1) != OPR1)
		goto inval; /* not an OPR instruction */
	else if ((op & 00400) == 0)
		opr1(op);   /* OPR group 1 */
	else if ((op & 00007) == 0)
		opr2(op);   /* OPR group 2 */
	else
		goto inval; /* OPR group 3 or privileged */

inval:	fatal(NULL, "invalid arg to %s: %06o", __func__, op);
}
