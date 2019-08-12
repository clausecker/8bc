/* codegen.c -- PDP-8 code generation */

#include <stdio.h>
#include <string.h>

#include "param.h"
#include "pdp8.h"
#include "asm.h"
#include "codegen.h"
#include "error.h"
#include "data.h"
#include "name.h"

/*
 * Labels related to the current function.
 *
 * framelabel
 *     call frame area
 *
 * paramlabel
 *     beginning of the parameter area
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
static struct expr paramlabel = { 0, "(param)" };
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
		if (frametmpl[i] == (v & ~LMASK))
			goto found;

	/* not found */
	if (nframe >= NSCRATCH)
		fatal(NULL, "frame overflow");

	frametmpl[nframe++] = v & ~LMASK;

found:	r.value = MINSCRATCH + i | RVALUE | v & LMASK;
	return (r);
}

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
		sprintf(buf, "L%04o+%03o", val(paramlabel.value), val(v));
		break;

	default:
		fatal(e->name, "invalid arg to %s: %06o", __func__, v);
	}

	return (buf);
}

extern void
emitl(const struct expr *e)
{
	struct expr spill = { 0, "(spill)" };

	switch (class(e->value)) {
	/* spill needed to construct address */
	case RCONST:
	case RLABEL:
	case RDATA:
	case RAUTO:
	case RPARAM:
		literal(&spill, e);
		e = &spill;
		break;

	/* no spill needed */
	default:
		;
	}

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

	if ((op & IAC) == IAC)
		strcat(buf, "IAC ");

	/* can't have RAL and RAR set at the same time */
	if ((op & (RAL | RAR)) == (RAL | RAR))
		return (0);

	/* inspect RAL, RAR, and BSW */
	strncat(buf, rots[op >> 1 & 7], 4);

	return (1);
}

/*
 * Generate a group 2 microcoded instruction.  Up to four instructions
 * may be generated:
 *
 *    any of SMA SZA SNL
 * or any of SPA SNA SZL
 * or SKP
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
	int i, skp;

	skp = (op & SKP) == SKP;

	for (i = 0; i < 3; i++)
		if (op & 00020 << i)
			strncat(buf, mnemo[skp][i], 4);

	/* if skp but no bit is set, emit SKP */
	if ((op & (SPA | SNA | SZL)) == SKP)
		strcpy(buf, "SKP ");

	if ((op & CLA) == CLA)
		strcat(buf, "CLA ");

	return (1);
}

/*
 * Emit the given OPR instruction.
 */
static void
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

	if (succeeded && (op & (SMA | SZA | SNL)) > OPR2)
		skip();
}

extern void
emitpush(struct expr *e)
{
	e->value = ++tos | RSTACK;
	memset(e->name, 0, MAXNAME);

	if (tos >= stacksize) {
		stacksize = tos + 1;
		if (stacksize > NSCRATCH)
			error(NULL, "stack overflow");
	}
}

extern void
emitpop(struct expr *e)
{
	if (!onstack(e->value))
		return;

	if (val(e->value) != tos--)
		fatal(NULL, "can only pop top of stack");

	e->value = EXPIRED;
}

extern void
ret(void)
{
	jmp(&retlabel);
}

extern void
newframe(const struct expr *fun)
{
	newlabel(&framelabel);
	newlabel(&paramlabel);
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
	emitc(0);
	commentname(fun->name);
	instr("ENTER");
	emitl(&framelabel);

	acclear();
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

extern void endframe(const struct expr *fun)
{
	struct expr dummy = { 0, "(dummy)" };
	int i, nsave;

	/* function epilogue */
	putlabel(&retlabel);
	instr("LEAVE");
	emitl(fun);

	blank();

	/* function metadata */
	setlabel(&stacklabel);
	emitc(nframe + MINSCRATCH);

	putlabel(&framelabel);

	/* saved registes area */
	nsave = nframe + stacksize;
	emitc(-nsave);
	comment("SAVE %04o REGISTERS", nsave);
	advance(nsave);

	/* parameter area */
	emitc(-nparam);
	comment("LOAD %04o ARGUMENTS", nparam);
	if (nparam > 0) {
		putlabel(&paramlabel);
		advance(nparam);
	}

	/* frame template */
	emitc(-nframe);
	comment("LOAD %04o TEMPLATES", nframe);
	for (i = 0; i < nframe; i++) {
		dummy.value = frametmpl[i];
		emitr(&dummy);
	}

	/* automatic variable area */
	if (nauto > 0) {
		putlabel(&autolabel);
		advance(nauto);
	}
}

extern void
emitisn(int isn, const struct expr *e)
{
	static const char mnemo[6][4] = { "AND", "TAD", "ISZ", "DCA", "JMS", "JMP" };
	int skp = 0;
	char buf[5];

	switch (isn & 07000) {
	case IOT:
		sprintf(buf, "%04o", isn & 07777);
		fatal(buf, "IOT not supported");
		break;

	case OPR:
		emitopr(isn);
		break;

	case ISZ:
		skp = 1;
		/* FALLTHROUGH */

	default:
		instr("%s %s", mnemo[isn >> 9 & 7], arg(e));
		commentname(e->name);
		if (skp)
			skip();
	}
}
