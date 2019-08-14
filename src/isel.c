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
 * The content of the L:AC register and a byte telling us what we
 * know about the contents of L and AC.  Two ac structures are kept
 * track of: the have state is the state AC is in before deferred
 * instructions are applied, want is after deferred instructions are
 * applied.
 */
enum {
	/* known values */
	LKNOWN = 1 << 0,  /* L is known to hold the value in lac */
	LANY = 1 << 1,    /* we don't care what value L holds */
	ACKNOWN = 1 << 2, /* AC is known to hold the value in lac */
};

static struct ac {
	unsigned short lac;
	unsigned char known;
} have = { 0, ACKNOWN | LANY }, want = { 0, ACKNOWN | LANY };

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
 * Find a sequence of up to 2 OPR instructions that produce lac
 * in L:AC.  If such a sequence is found, defer it and return 1.
 * Otherwise, return 0.
 */
static int
findseq(int lac)
{
	/* TODO */
	return (0);
}

/*
 * Assuming what the deferred instructions do is just computing
 * constants, fold the computations into at most 2 instructions.
 *
 * invariant: if ACKNOWN or LKNOWN are set in have, they are also
 * set in want.
 */
static void
fold(void)
{
	/*
	 * sequence of instruction to generate constants such that
	 * L is preserved.  The first entry is the constant to be
	 * generated, the other two entries are the two OPR instructions
	 * used (0 if there is only one).  The table is terminated with
	 * a 0 as the first instruction.
	 */
	static const unsigned short lpresseq[][3] = {
		00000, CLA,             0,
		00001, CLA | IAC,       0,
		00002, CLA | IAC,       IAC,
		00003, CLA | IAC | RAR, IAC | RAL,
		00004, CLA | RTR,       IAC | RTL,
		00006, CLA | RTR,       STL | IAC | RTL,
		02000, CLA | RTL,       STL | RTR,
		03777, STA | RAL,       CLL | RAR,
		04000, CLA | RAL,       STL | RAR,
		06000, CLA | RTL,       STL | IAC | RTR,
		06777, STA | RTL,       CLL | RTR,
		07775, STA | RTR,       CLL | RTL,
		07776, STA | RAR,       CLL | RAL,
		07777, STA,             0,
		00000, 0,               0,
	};

	int wantac, haveac, i;
	struct expr e = { 0, "" };

	/* discard deferred instructions */
	ndefer = 0;

	wantac = want.lac & 07777;
	haveac = have.lac & 07777;

	/* AC already set up? */
	if (~want.known & ACKNOWN || have.known & ACKNOWN && wantac == haveac) {
		/* need to set up L? */
		if (want.known & LKNOWN && ~want.known & LANY)
			defer(want.lac & 010000 ? STL: CLL, NULL);

		return;
	}

	/* otherwise, what about L? */
	switch (want.known & (LKNOWN | LANY)) {
	case 0:	/* nothing known, must be preserved: produce AC without touching L */
		for (i = 0; lpresseq[i][1] != 0; i++)
			if (lpresseq[i][0] == wantac)
				goto seqfound;

		if (have.known & ACKNOWN && haveac <= wantac) {
			e.value = RCONST | wantac - haveac;
			defer(TAD, &e);
		} else if (have.known & ACKNOWN && (~haveac & wantac) == 0) {
			e.value = RCONST | wantac;
			defer(AND, &e);
		} else {
			defer(CLA, NULL);
			e.value = RCONST | wantac;
			defer(TAD, &e);
		}

		break;

	seqfound:
		defer(lpresseq[i][1], NULL);
		if (lpresseq[i][2] != 0)
			defer(lpresseq[i][2], NULL);

		break;

	case LANY:
	case LKNOWN | LANY: /* don't care about L */
		if (findseq(wantac &~ 010000) || findseq(wantac | 010000)) {
			want.known |= LKNOWN;
			break;
		}

		if (have.known & ACKNOWN) {
			e.value = RCONST | wantac - haveac;
			defer(TAD, &e);
			want.lac = have.lac + wantac - haveac & 017777;
			want.known &= ~LKNOWN;
			want.known |= have.known & LKNOWN;
		} else {
			defer(CLA | CLL, NULL);
			e.value = RCONST | wantac;
			defer(TAD, &e);
			want.lac &= ~010000;
			want.known |= LKNOWN;
		}

		break;

	case LKNOWN: /* must set L appropriate */
		if (findseq(want.lac))
			break;

		/* if AC is known, we need to check if adding wantac - haveac flips L correctly */
		if (have.known & ACKNOWN) {
			/* preserve L or flip L? */
			if ((want.lac & 010000) == (have.lac & 010000)) {
				if (haveac <= wantac) {
					e.value = RCONST | wantac - haveac;
					defer(TAD, &e);
				} else if ((~haveac & wantac) == 0) {
					e.value = RCONST | wantac;
					defer(AND, &e);
				} else
					goto cla_tad_sequence;
			} else if (haveac > wantac) {
				e.value = RCONST | wantac - haveac;
				defer(TAD, &e);
			} else
				goto cla_tad_sequence;
		} else {
		cla_tad_sequence:
			defer(want.lac & 010000 ? CLA | STL : CLA | CLL, NULL);
			e.value = RCONST | wantac;
			defer(TAD, &e);
		}
	}
}

/*
 * TODO: document
 */
static void
normalsel(int op, const struct expr *e)
{
	/*
	 * must_emit contains 0 if all effects of op can be computed by
	 * the optimiser, 1 otherwise.  If must_emit & 1, we have
	 * to emit all deferred instructions as well as op.  If
	 * must_emit & 2, we also need to set acstate to 2.
	 */
	int must_emit = 0;
	int v;

	v = e != NULL ? e->value : INVALID;

	switch (op & 07000) {
	case AND:
		if (want.known & ACKNOWN && isconst(v))
			want.lac &= 010000 | val(v);
		else {
			must_emit |= 3;
			want.known &= ~ACKNOWN;
		}

		break;

	case TAD:
		if (want.known & ACKNOWN && isconst(v))
			want.lac = 017777 & want.lac + val(v);
		else {
			must_emit |= 3;
			if (want.known & LKNOWN && (~want.known & ACKNOWN || (want.lac & 007777) != 0))
				want.known &= ~LKNOWN;

			want.known &= ~ACKNOWN;
		}

		break;

	case ISZ:
		skipstate = SKIPABLE;
		must_emit |= 1;
		break;

	case DCA:
		must_emit |= 1;
		want.lac &= 010000;
		want.known |= ACKNOWN;
		break;

	case JMP:
		must_emit |= 1;
		break;

	case OPR: {
		struct ac will;
		int o;

		will = want;
		o = op;

		for (;;) switch (peelopr(&o)) {
		case CLA:
			will.lac &= ~007777;
			will.known |= ACKNOWN;
			break;

		case CLL:
			will.lac &= ~010000;
			will.known |= LKNOWN;
			will.known &= ~LANY;
			break;

		case CMA:
			if (will.known & ACKNOWN)
				will.lac ^= 007777;
			else
				must_emit |= 3;
			break;

		case CML:
			if ((will.known & (LKNOWN | LANY)) == 0)
				must_emit |= 1;
			else
				will.lac ^= 010000;
			break;

		case RAR:
			if (will.known & ACKNOWN && will.known & LKNOWN) {
				will.lac = will.lac >> 1 | will.lac << 12 & 010000;
				will.known &= ~LANY;
			} else {
				will.known = 0;
				must_emit |= 3;
			}

			break;

		case RTR:
			if (will.known & ACKNOWN && will.known & LKNOWN) {
				will.lac = will.lac >> 2 | will.lac << 11 & 014000;
				will.known &= ~LANY;
			} else {
				will.known = 0;
				must_emit |= 3;
			}

			break;

		case RAL:
			if (will.known & ACKNOWN && will.known & LKNOWN) {
				will.lac = will.lac << 1 & 017776 | will.lac >> 12;
				will.known &= ~LANY;
			} else {
				will.known = 0;
				must_emit |= 3;
			}

			break;

		case RTL:
			if (will.known & ACKNOWN && will.known & LKNOWN) {
				will.lac = will.lac << 2 & 017774 | will.lac >> 11;
				will.known &= ~LANY;
			} else {
				will.known = 0;
				must_emit |= 3;
			}

			break;

		case IAC:
			if (will.known & ACKNOWN) {
				if (~will.known & LKNOWN && (will.lac & 07777) == 07777)
					must_emit |= 1;

				will.lac = will.lac + 1 & 017777;
			} else
				must_emit |= 3;
			break;

		case SMA:
			if (skipstate == DOSKIP)
				break;

			if (~will.known == ACKNOWN) {
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

			if (~will.known & ACKNOWN) {
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

			if (~will.known & LKNOWN) {
				skipstate = SKIPABLE;
				must_emit = 1;
			} else if (will.lac & 010000) {
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

		/* figure out if the instruction was a no-op.  If yes, ignore it. */
	peeled:	if (!must_emit && skipstate == NORMAL
		    && (want.known & (LKNOWN | ACKNOWN)) == (will.known & (LKNOWN | ACKNOWN))
		    && (~will.known & LKNOWN || (want.lac & 010000) == (will.lac & 010000))
		    && (~will.known & ACKNOWN || (want.lac & 007777) == (will.lac & 007777)))
			return;

		want = will;

		break;
	}


	default:
		fatal(NULL, "invalid arg to %s: %06o", __func__, op);
	}

	if (must_emit & 1) {
		undefer();
		emitisn(op, e);
	/* non-skipping instruction */
	} else if (skipstate == NORMAL) {
		defer(op, e);
		fold();
	/* skipping instruction */
	} else
		defer(op, e);

	if (want.known & ACKNOWN) {
		acstate = zero;
		acstate.value = RCONST | want.lac & 007777;
	} else if (must_emit & 2)
		acstate = random;
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
	want.known = 0;
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
		want.known = LANY | ACKNOWN;
		have = want;
		skipstate = NORMAL;
		return;

	case RND:
		want.known = LANY;
		undefer();
		return;

	case LIV:
		want.known |= LANY;
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
