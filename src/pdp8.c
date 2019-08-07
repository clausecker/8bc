/* pdp8.c -- instruction selection */

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
 * Machine state.
 *
 * acstate
 *     The expression whose value should currently be in AC.  If the
 *     value of AC should not correspond to any expression, it is set
 *     to RANDOM.
 *
 * dirty
 *     This flag is 1 if dca(acstate) is deferred.  It is set by push(),
 *     cleared by pop(), writeback(), and acclear() and carefully
 *     managed by lda().
 */
static struct expr acstate = { RCONST | 0, "" };
char dirty = 0;

/* acstate templates */
static const struct expr zero = { RCONST | 0, "" };
static const struct expr random = { RANDOM, "" };

/* forward declarations */
static void writeback(void);
static void isel(int, const struct expr *);

extern void
push(struct expr *e)
{
	if (acstate.value != RANDOM && !onstack(acstate.value)) {
		*e = acstate;
		return;
	}

	writeback();
	emitpush(e);
	acstate = *e;
	dirty = 1;
}

extern void
pop(struct expr *e)
{
	/* omit writeback on immediate pop */
	if (acstate.value == e->value)
		dirty = 0;

	emitpop(e);
}

/*
 * if dirty is set, write the content of AC to acstate and clear the
 * dirty flag.
 */
static void
writeback(void)
{
	if (dirty) {
		struct expr e = acstate;

		isel(DCA, &e);
		dirty = 0;
		isel(TAD, &e);
	}
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
	/* strength reduction */
	if (e->value == (RCONST | 07777)) {
		isel(NOP, NULL);
		return;
	}

	writeback();

	if (e->value == (RCONST | 00000))
		isel(CLA, NULL);
	else
		isel(AND, e);
}

extern void
tad(const struct expr *e)
{
	/* strength reduction */
	if (e->value == (RCONST | 00000)) {
		isel(NOP, NULL);
		return;
	}

	writeback();
	if (acstate.value != RANDOM && e->value == acstate.value)
		isel(CLL | RAL, NULL);
	else
		isel(TAD, e);
}

extern void
isz(const struct expr *e)
{
	writeback();
	emitisn(ISZ, e);
}

extern void
dca(const struct expr *e)
{
	writeback();
	isel(DCA, e);
}

extern void
jms(const struct expr *e)
{
	catchup();
	emitisn(JMS, e);
	isel(RND, NULL);
}

extern void
jmp(const struct expr *e)
{
	catchup();
	emitisn(JMP, e);
}

extern void
lda(const struct expr *e)
{
	/* omit duplicate loads */
	if (acstate.value == e->value)
		return;

	isel(LDA, e);
}

extern void
opr(int op)
{
	/* simplify case distinction */
	switch (op) {
	case OPR2:
		op = NOP;
		/* FALLTHROUGH */

	case NOP:
		break;

	case OPR2 | CLA:
		op = CLA;
		/* FALLTHROUGH */

	default:
		writeback();
	}

	isel(op, NULL);
}

extern void
acclear(void)
{
	dirty = 0;
	isel(RST, NULL);
}

extern void
catchup(void)
{
	writeback();
	isel(CUP, NULL);
}

/*
 * Peel instructions off op until NOP remains.  The instructions are
 * returned in the following order:
 *
 * group 1: CLA, CLL, CMA, CML, IAC, RAR/RAL/RTR/RTL/BSW
 * group 2: SMA, SZA, SNL, SKP, CLA
 *
 * the returned micro instruction is then stripped off op.  When no bit
 * remains, NOP is returned.
 */
static int
peelopr(int *op)
{
	int i, uop;
	static const unsigned short
	    opr1tab[] = { CLA, CLL, CMA, CML, IAC, RTR | RTL, 0 },
	    opr2tab[] = { SMA, SZA, SNL, SKP, CLA, 0 }, *oprtab;

	oprtab = *op & 00400 ? opr2tab : opr1tab;

	for (i = 0; oprtab[i] != 0; i++) {
		uop = *op & oprtab[i];
		if (*op & oprtab[i] & 00377) {
			*op &= ~oprtab[i] | 07400;
			return (uop);
		}
	}

	return (NOP);
}

/*
 * instruction selection state machine.
 *
 * Compute the effect of instruction op with operand e and remember it.
 * Update acstate to the new content of AC.  Possible emit code.
 *
 * Three pseudo instructions exist to manipulate the state machine:
 *
 * CUP "catch up" -- emit all deferred instructions such that the
 *     the current machine state corresponds to the simulated state.
 *     This pseudo-instruction is emitted whenever isel is circumvented
 *     to emit code or data into the assembly file directly.
 *
 * RST "reset" -- forget all deferred instructions and reset the state
 *     machine to a clear accumulator.  This is used at the beginning of
 *     functions and after labels that are only reachable by jumping to
 *     them.
 *
 * RND "AC state random" -- forget any knowledge about what AC contains
 *     and assume that its contents are unpredictable.  This is used
 *     whenever isel is circumvented such that AC may have been
 *     modified.
 */
enum { MAXDEFER = 10, }; /* dummy value */
static void
isel(int op, const struct expr *e)
{
	/* an instruction with a corresponding operand. */
	struct isn {
		struct expr e;
		unsigned short op;
	};

	/*
	 * deferred instructions.  If these instructions were emitted,
	 * the state of the machine would match the expected state.
 	 */
	static struct isn deferrals[MAXDEFER];
	static unsigned char ndeferred = 0;

	/* machine states */
	enum {
		ACCONST, /* AC holds a constant, L random */
		ACRANDOM, /* AC and L hold unknown values */
		SKIPABLE, /* next instruction might be skipped */
	};

	static unsigned char state = ACCONST, skipstate;
	static unsigned short lac;
	static unsigned char needsclear = 0, ldefined = 0;

	/* first, handle pseudo instructions */
	switch (op) {
	case CUP:
		goto flush;

	case RST:
		needsclear = 0;
		ndeferred = 0;
		state = ACCONST;
		lac = 0;
		acstate = zero;
		return;

	case RND:
		ndeferred = 0;
		state = ACRANDOM;
		acstate = random;
		return;

	default:
		/* not a pseudo instruction */
		;
	}

	/* defer current instruction */
	deferrals[ndeferred].op = op;
	deferrals[ndeferred].e = e != NULL ? *e : zero;

	/* process state machine */
	switch (state) {
	/*
	 * Invariant: 1--3 instruction deferred.  The deferred
	 * instruction do the following:
	 *
	 * 1. if needsclear, AC is cleared.
	 * 2. lac is loaded into L:AC.
	 * 3. op/e is executed.
	 */
	case ACCONST:
		switch (op & 07000) {
		case AND:
			if (!isconst(e->value))
				goto indeterminate;

			lac &= 010000 | e->value & 07777;
			break;

		case TAD:
			if (!isconst(e->value))
				goto indeterminate;

			lac += e->value & 07777;
			lac &= 017777;
			break;

		case ISZ:
			skipstate = state;
			state = SKIPABLE;
			return;

		case DCA:
			/* TODO: handling applies to most states */
			needsclear = 0;
			ldefined = 0;
			state = ACCONST;
			lac = 0;
			goto flush;

		case JMP:
			goto flush;

		case OPR:
			for (;;) switch (peelopr(&op)) {
			case NOP:
				break;
			}
		}

		/* TODO */

	case ACRANDOM:
	case SKIPABLE:
		/* TODO */
		abort();
	}
}
