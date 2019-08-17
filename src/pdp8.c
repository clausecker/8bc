/* pdp8.c -- high level ASM interface */

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
struct expr acstate = { RCONST | 0, "" };
static char dirty = 0;

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
	}
}

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
	isel(LIV, NULL);
}

extern void
pop(struct expr *e)
{
	/* omit writeback on immediate pop */
	if (acstate.value == e->value)
		dirty = 0;

	emitpop(e);
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
ldconst(int c)
{
	struct expr e = { 0, "" };

	e.value = c;
	lda(&e);
}

extern void
lda(const struct expr *e)
{
	/* omit duplicate loads */
	if (acstate.value == e->value)
		return;

	writeback();
	isel(CLA, NULL);
	isel(TAD, e);
	isel(LIV, NULL);
}

extern void
opr(int op)
{
	/* simplify case distinction */
	if (op != NOP && op != OPR2)
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
