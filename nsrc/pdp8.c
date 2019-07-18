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
 *
 * retlabel
 *     points to the current function's leave instruction
 */
static struct expr framelabel = { 0, "(frame)" };
static struct expr stacklabel = { 0, "(stack)" };
static struct expr autolabel = { 0, "(auto)" };
static struct expr retlabel = { 0, "(return)" };

/*
 * Stack variables.
 *
 * stacksize
 *     the maximum amount of stack registers used in the current
 *     function
 *
 * tos
 *     the last stack register pushed
 */
static unsigned char stacksize;
static signed char tos;

/*
 * Call frame variables.
 *
 * nparam
 *     number of function parameters
 *
 * nauto
 *     number of automatic variables
 *
 * nframe
 *     number of frame registers
 *
 * frametmpl
 *     frame register template
 */
static unsigned char nparam, nauto, nframe;
static unsigned short frametmpl[NSCRATCH];

/* forward declarations */
static struct expr spill(const struct expr *);

/*
 * Generate a string representation of the address of e as needed for
 * emitl.  e must be of type LCONST, RVALUE, LLABEL, RSTACK,
 * RAUTO, or RPARAM.  The returned string is placed in a static buffer.
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
		sprintf(buf, "L%04o", val(v));
		break;

	case LDATA:
		sprintf(buf, "DATA+%04o", val(v));
		break;

	case RSTACK:
		sprintf(buf, "L%04o+%03o", val(stacklabel.value), val(v));
		break;

	case LAUTO:
		sprintf(buf, "L%04o+%03o", val(autolabel.value), val(v));
		break;

	case LPARAM:
		sprintf(buf, "L%04o+%03o", val(framelabel.value), val(v) + 2);
		break;

	default:
		fatal(e->name, "invalid arg to %s: %06o", __func__, v);
	}

	return (buf);
}

extern void
emitl(const struct expr *e)
{
	instr(lstr(e));
}

extern void
emitr(const struct expr *e)
{
	struct expr le;

	switch (class(e->value)) {
	case RCONST:
	case RLABEL:
	case RDATA:
	case RAUTO:
	case RPARAM:
		le = r2lval(e);

		instr(lstr(&le));
		break;

	default:
		fatal(e->name, "invalid arg to %s: %06o", __func__, e->value);
	}
}

extern void
skip(int n)
{
	if (n > 0)
		instr("*.+%04o", n);
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
	struct expr sp;
	static char buf[16];

	sp = spill(e);
	if (islval(sp.value)) {
		sp = l2rval(&sp);
		sprintf(buf, "I %s", lstr(&sp));
	} else
		strcpy(buf, lstr(&sp));

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

	return;

inval:	fatal(NULL, "invalid arg to %s: %06o", __func__, op);
}

extern void
push(struct expr *e)
{
	e->value = ++tos | RSTACK;
	memset(e->name, 0, MAXNAME);

	if (tos > stacksize) {
		stacksize = tos;
		if (stacksize > NSCRATCH)
			error(NULL, "stack overflow");
	}

	dca(e);
}

extern void
pop(struct expr *e)
{
	if (!onstack(e->value))
		return;

	if (val(e->value) != tos--)
		fatal(NULL, "can only pop top of stack");

	e->value = EXPIRED;
}

/*
 * Allocate a frame register for expr and return it.  If expr is
 * of type RVALUE, LVALUE, RSTACK, or LSTACK,  return it unchanged.
 * Otherwise the result always has type RVALUE or LVALUE.
 */
static struct expr
spill(const struct expr *e)
{
	struct expr r = { 0, "" };
	int i, v = e->value;

	switch (class(v)) {
	case RVALUE:
	case LVALUE:
	case RSTACK:
	case LSTACK:
		r = *e;
		return (r);

	case LCONST:
		/* don't spill zero page addresses */
		if (val(v) < NZEROPAGE) {
			memcpy(r.name, e->name, MAXNAME);
			r.value = RVALUE | val(v);
			return (r);
		} else
			break;

	default:
		;
	}

	/* was v spilled before? */
	for (i = 0; i < nframe; i++)
		if (frametmpl[i] == v)
			goto found;

	/* not found */
	if (nframe >= NSCRATCH)
		fatal(NULL, "frame overflow");

	frametmpl[nframe++] = v;

found:	r.value = MINSCRATCH + i | RVALUE | v & LMASK;
	return (r);
}

extern void
newframe(struct expr *fun)
{
	newlabel(&framelabel);
	newlabel(&stacklabel);
	newlabel(&autolabel);
	newlabel(&retlabel);

	tos = -1;
	stacksize = 0;

	nparam = 0;
	nauto = 0;
	nframe = 0;
	endscope(0);

	/* function prologue */
	skip(1);
	instr("ENTER");
	emitl(&framelabel);
}

extern void
newparam(struct expr *par)
{
	par->value = LPARAM | nparam++;
}

extern void
newauto(struct expr *var)
{
	var->value = LAUTO | nauto++;
}

extern void
ret(void)
{
	jmp(&retlabel);
}

extern void endframe(void)
{
	putlabel(&retlabel);
	instr("LEAVE");
}
