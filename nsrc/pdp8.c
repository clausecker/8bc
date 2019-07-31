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
 * achave
 *     The expression whose value is currently in AC.  If the value
 *     of AC does not correspond to any expression, it is set to
 *     RANDOM.
 *
 * acwant
 *     The expression whose value should currently be in AC.  If the
 *     value of AC should not correspond to any expression, it is set
 *     to RANDOM.
 *
 * lwant, lhave
 *     The current and desired value of the link bit.  This should be
 *     one of LCLEAR, LSET, LRANDOM.
 *
 * skipstate
 *     The current state of the code generation.  Documented in the
 *     enumeration below.  This is orthogonal to the achave/acwant
 *     mechanism.
 *
 * dirty
 *     This flag is 1 if AC contents need to be deposited into acwant.
 *     If dirty is set, achave == acwant.
 */
/* AC state enumeration */
enum {
	CURRENT,  /* nothing deferred (may still need to update AC to acwant) */
	SKIPABLE, /* next instruction could be skipped, do not alter */
	SKIPPED,  /* do not emit next instruction */
};

/* L state enumeration */
enum {
	LRANDOM,  /* L bit has unknown value */
	LCLEAR,   /* L bit is known to be clear */
	LSET,     /* L bit is known to be set */
};

static struct expr achave = { RCONST | 0, "" };
static struct expr acwant = { RCONST | 0, "" };
char lwant = LRANDOM, lhave = LRANDOM, acstate = CURRENT, dirty = 0;

/* AC value templates */
static const struct expr zero = { RCONST | 0, "" };
static const struct expr random = { RANDOM, "" };

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
	catchup();
	emitand(e);
	acwant = achave = random;
}

extern void
tad(const struct expr *e)
{
	catchup();
	emittad(e);
	acwant = achave = random;
}

extern void
isz(const struct expr *e)
{
	catchup();
	emitisz(e);
	acwant = achave = random;
}

extern void
dca(const struct expr *e)
{
	catchup();
	emitdca(e);
	acwant = achave = random;
}

extern void
jms(const struct expr *e)
{
	catchup();
	emitjms(e);
	acwant = achave = random;
}

extern void
jmp(const struct expr *e)
{
	catchup();
	emitjmp(e);
}

extern void
lda(const struct expr *e)
{
	/* omit duplicate loads */
	if (acwant.value == e->value)
		return;

	catchup();
	opr(CLA);
	tad(e);
	acwant = achave = *e;
}

extern void
opr(int op)
{
	catchup();
	emitopr(op);
	acwant = achave = random;
}

extern void
push(struct expr *e)
{
	if (acwant.value != RANDOM && !onstack(acwant.value)) {
		*e = acwant;
		return;
	}

	emitpush(e);
	achave = acwant = *e;
	dirty = 1;
}

extern void
pop(struct expr *e)
{
	if (acwant.value == e->value)
		dirty = 0;

	emitpop(e);
}

extern void
acclear(void)
{
	achave = acwant = zero;
	lwant = lhave = LRANDOM;
	acstate = CURRENT;
}

extern void
catchup(void)
{
	if (dirty) {
		emitdca(&acwant);
		dirty = 0;
	}

	/* todo: actually catch up */
}
