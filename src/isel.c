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
 * The content of the L:AC register and two bytes telling us what we
 * know about the contents of L and AC.  Two ac structures are kept
 * track of: the have state is the state AC is in before deferred
 * instructions are applied, want is after deferred instructions are
 * applied.
 */
static struct ac {
	unsigned short lac;
	unsigned char lknown, acknown;
} have = { 0, 0, 1 }, want = { 0, 0, 1 };

enum {
	/* lknown values */
	LRANDOM = 0, /* L holds a random value */
	LKNOWN = 1,  /* L holds the value in lac */
	LANY = 2,    /* we don't care what value L holds */

	/* acknown values */
	ACRANDOM = 0, /* AC holds a random value */
	ACKNOWN = 1, /* AC holds the value in lac */
};

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
		if (uop & 00377) {
			*op &= ~oprtab[i] | 07400;
			return (uop);
		}
	}

	return (NOP);
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

/*
 * Push op/e onto the list of deferred instructions.
 */
static void
defer(int op, const struct expr *e)
{
	/* this should never happen */
	if (ndefer == MAXDEFER) {
		warn(NULL, "defer stack overflow");
		undefer();
	}

	deferred[ndefer].op = op;
	deferred[ndefer].e = *e;
	ndefer++;
}

/*
 * Assuming what the deferred instructions do is just computing
 * constants, fold the computations into at most 2 instructions.
 */
static void
fold(void)
{
	/* TODO */
}

/*
 * TODO: document
 */
static void
normalsel(int op, const struct expr *e)
{
	/*
	 * must_emit contains 0 if all effects of op can be computed by
	 * the optimiser, 1 otherwise.  If must_emit is 1, we have
	 * to emit all deferred instructions as well as op.
	 */
	int must_emit = 0;
	int v;

	v = e != NULL ? e->value : INVALID;

	switch (op & 07000) {
	case AND:
		if (want.acknown == ACKNOWN && isconst(v))
			want.lac &= 010000 | val(v);
		else {
			acstate = random;
			must_emit = 1;
			want.acknown = ACRANDOM;
		}

		break;

	case TAD:
		if (want.acknown == ACKNOWN && isconst(v))
			want.lac = 017777 & want.lac + val(v);
		else {
			acstate = random;
			must_emit = 1;
			if (want.lknown == LKNOWN && (want.acknown != ACKNOWN || (want.lac & 007777) != 0))
				want.lknown = LRANDOM;

			want.acknown = ACRANDOM;
		}

		break;

	case ISZ:
		skipstate = SKIPABLE;
		must_emit = 1;
		break;

	case DCA:
		must_emit = 1;
		want.lac &= 010000;
		want.acknown = ACKNOWN;
		break;

	case JMP:
		must_emit = 1;
		break;

	case OPR: {
		struct ac will;
		int o;

		will = want;
		o = op;

		for (;;) switch (peelopr(&o)) {
		case CLA:
			will.lac &= ~007777;
			will.acknown = ACKNOWN;
			break;

		case CLL:
			will.lac &= ~010000;
			will.lknown = LKNOWN;
			break;

		case CMA:
			if (will.acknown == ACKNOWN)
				will.lac ^= 007777;
			else
				must_emit = 1;
			break;

		case CML:
			if (will.lknown == LRANDOM)
				must_emit = 1;
			else
				will.lac ^= 010000;
			break;

		case RAR:
			if (will.acknown == ACKNOWN && will.lknown != LRANDOM) {
				will.lac = will.lac >> 1 | will.lac << 12 & 010000;
				will.lknown = LKNOWN;
			} else {
				will.acknown = ACRANDOM;
				will.lknown = LRANDOM;
				must_emit = 1;
			}

			break;

		case RTR:
			if (will.acknown == ACKNOWN && will.lknown != LRANDOM) {
				will.lac = will.lac >> 2 | will.lac << 11 & 014000;
				will.lknown = LKNOWN;
			} else {
				will.acknown = ACRANDOM;
				will.lknown = LRANDOM;
				must_emit = 1;
			}

			break;

		case RAL:
			if (will.acknown == ACKNOWN && will.lknown != LRANDOM) {
				will.lac = will.lac << 1 & 017776 | will.lac >> 12;
				will.lknown = LKNOWN;
			} else {
				will.acknown = ACRANDOM;
				will.lknown = LRANDOM;
				must_emit = 1;
			}

			break;

		case RTL:
			if (will.acknown == ACKNOWN && will.lknown != LRANDOM) {
				will.lac = will.lac << 2 & 017774 | will.lac >> 11;
				will.lknown = LKNOWN;
			} else {
				will.acknown = ACRANDOM;
				will.lknown = LRANDOM;
				must_emit = 1;
			}

			break;

		case IAC:
			if (will.acknown == ACKNOWN) {
				if (will.lknown == LRANDOM && (will.lac & 07777) == 07777)
					must_emit = 1;

				will.lac = will.lac + 1 & 017777;
			} else
				must_emit = 1;
			break;

		case SMA:
			if (skipstate == DOSKIP)
				break;

			if (will.acknown == ACRANDOM) {
				skipstate = SKIPABLE;
				must_emit = 1;
			} else if (will.lac & 004000) {
				skipstate = DOSKIP;
				must_emit = 0;
			}

			break;

		case SZA:
			if (skipstate == DOSKIP)
				break;

			if (will.acknown == ACRANDOM) {
				skipstate = SKIPABLE;
				must_emit = 1;
			} else if ((will.lac & 007777) == 0) {
				skipstate = DOSKIP;
				must_emit = 0;
			}

			break;

		case SNL:
			if (skipstate == DOSKIP)
				break;

			if (will.lknown == LRANDOM) {
				skipstate = SKIPABLE;
				must_emit = 1;
			} else if ((will.lac & 010000)) {
				skipstate = DOSKIP;
				must_emit = 0;
			}

			break;

		case SKP:
			switch (skipstate) {
			case NORMAL: skipstate = DOSKIP; break;
			case SKIPABLE: break;
			case DOSKIP: skipstate = NORMAL; break;
			}

			break;

		case NOP:
			goto peeled;

		default:
			fatal(NULL, "unregonised OPR instruction: %04o", op & 07777);
		}

	peeled:	/* figure out if the instruction was a no-op.  If yes, ignore it. */
		if (!must_emit && skipstate == NORMAL
		    && want.lknown == will.lknown
		    && want.acknown == will.acknown
		    && (will.lknown != LKNOWN || (want.lac & 010000) == (will.lac & 010000))
		    && (will.acknown != ACKNOWN || (want.lac & 007777) == (will.lac & 007777)))
			return;

		want = will;
		break;
	}


	default:
		fatal(NULL, "invalid arg to %s: %06o", __func__, op);
	}

	if (must_emit) {
		undefer();
		emitisn(op, e);
	/* non-skipping instruction */
	} else if (skipstate == NORMAL) {
		defer(op, e);
		fold();
	/* skipping instruction */
	} else
		defer(op, e);

	if (want.acknown == ACKNOWN) {
		acstate = zero;
		acstate.value = RCONST | want.lac & 007777;
	}
}

/*
 * Select instructions when skipstate is SKIPABLE.  In this state,
 * we emit op unchanged but try to record its effect in want.
 */
static void
skipsel(int op, const struct expr *e)
{
	/* TODO: apply optimisations */
	acstate = random;
	skipstate = NORMAL;
	want.lknown = LRANDOM;
	want.acknown = ACRANDOM;
	undefer();
	emitisn(op, e);
}

extern void
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
		want.lknown = LANY;
		want.acknown = ACKNOWN;
		have = want;
		skipstate = NORMAL;
		return;

	case RND:
		want.lknown = LANY;
		want.acknown = ACRANDOM;
		undefer();
		return;

	case LIV:
		want.lknown = LANY;
		return;

	default:
		/* not a pseudo instruction */
		;
	}

	/* next, consider skip state */
	switch (skipstate) {
	case NORMAL:
		normalsel(op, e);
		break;

	case DOSKIP:
		/* discard skip and current instruction if possible */
		if (ndefer != 0) {
			ndefer--;
			skipstate = NORMAL;
			break;
		}

		/* if the skip instruction has already been emitted,
		 * treat op like a skipable instruction. */

		/* FALLTHROUGH */

	case SKIPABLE:
		skipsel(op, e);
		break;
	}
}
