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
	writeback();
	isel(JMP, e);
}

extern void
lda(const struct expr *e)
{
	/* omit duplicate loads */
	if (acstate.value == e->value)
		return;

	opr(CLA);
	isel(TAD, e);
}

extern void
opr(int op)
{
	if (op != NOP && op != (NOP | OPR2))
		writeback();

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
 * instruction selection state machine.
 *
 * Compute the effect of instruction op with operand e and remember it.
 * Update acstate to the new content of AC.  Possible emit code.
 *
 * Three pseudo-instructions exist to manipulate the state machine:
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
static void
isel(int op, const struct expr *e)
{
	static enum {
		ACCLEAR,	/* AC is clear, no deferred state */
		ACRANDOM,	/* AC holds an unknown value, no deferred state */
		ACCONST,	/* AC holds a constant, computing it is deferred */
		ACLCONST,	/* like ACCONST but L state is known */
		SKIP,		/* next instruction will be skipped, AC value known */
	} state = ACCLEAR;

	/* pseudo instruction? */
	if (op & 010000) {
		switch (op) {
		case CUP:
		case RST:
		case RND:
			/* TODO */
			break;

		default:
			fatal(NULL, "unknown pseudo instruction %07o", op);
		}

		return;
	}

	/* dummy */
	for (;;) switch (state) {
	default:
		acstate = random;
		emitisn(op, e);
		return;
	}
}
