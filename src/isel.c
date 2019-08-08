/* isel.c -- instruction selection */

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
 * The content of the L:AC register and two bytes telling us if the
 * content of L and AC is known.  If the content of these is unknown,
 * it must be accurately simulated.  Two ac structures are kept track
 * of: the have state is the state AC is in before deferred instructions
 * are applied, want is after deferred instructions are applied.
 */
static struct ac {
	unsigned short lac;
	unsigned char lknown, acknown;
} have = { 0, 0, 1 }, want = { 0, 0, 1 };

/*
 * The list of deferred instructions.  These instructions, when
 * executed, advance the system from have state to want state.
 * ndefer is the number of deferred instructions.
 */
enum { MAXDEFER = 10 };
struct instr {
	unsigned short op;
	struct expr e;
} deferred[MAXDEFER];
static signed char ndefer = 0;

/*
 * The current state of skippable instructions
 */
enum {
	NORMAL, /* not following a skip instruction */
	DOSKIP, /* following a skip instruction found to perform a skip */
	SKIPABLE, /* following a skip instruction that may skip */
	NOSKIP, /* following a skip instruction found not to perform a skip (unused?) */
};
static unsigned char skipstate = NORMAL;

/* acstate templates */
static const struct expr zero = { RCONST | 0, "" };
static const struct expr random = { RANDOM, "" };

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
 * Push op/e onto the list of deferred instructions.
 */
static void
defer(int op, struct expr *e)
{
	if (ndefer == MAXDEFER)
		fatal(NULL, "defer stack overflow);

	deferred[ndefer].op = op;
	deferred[ndefer].e = *e;
	ndefer++;
}

/*
 * Emit all deferred instructions and advance have to want.
 */
static void
undefer(void)
{
	int i;

	for (i = 0; i < ndefer; i++)
		emitisn(deferred[i].op, &deferred[i].e);

	ndefer = 0;
	have = want;
}

static void
isel(int op, const struct expr *e)
{
	/* first, catch special instructions */
	switch (op) {
	case CUP:
		undefer();
		return;

	case RST:
		ndefer = 0;
		want.lac = 0;
		want.lknown = 0;
		want.acknown = 1;
		have = want;
		skipstate = NORMAL;
		return;

	case RND:
		want.lknown = 0;
		want.acknown = 0;
		undefer();
		return;

	default:
		/* not a pseudo instruction */
		;
	}

	/* next, consider skip state */
	switch (skipstate) {
	case NORMAL:
	case DOSKIP:
	case SKIPABLE:
	}
}
