/* pdp8.c -- PDP-8 code generation */

#include <stdio.h>
#include <string.h>

#include "param.h"
#include "asm.h"
#include "error.h"
#include "pdp8.h"
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

extern void
opr(int op)
{
	emitopr(op);
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
		if (frametmpl[i] == (v & ~LMASK))
			goto found;

	/* not found */
	if (nframe >= NSCRATCH)
		fatal(NULL, "frame overflow");

	frametmpl[nframe++] = v & ~LMASK;

found:	r.value = MINSCRATCH + i | RVALUE | v & LMASK;
	return (r);
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
	skip(nsave);

	/* parameter area */
	emitc(-nparam);
	comment("LOAD %04o ARGUMENTS", nparam);
	skip(nparam);

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
		skip(nauto);
	}
}
