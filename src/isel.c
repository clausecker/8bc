/*- (c) 2019 Robert Clausecker <fuz@fuz.su> */
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
 * applied.  If we don't know the value of L or AC in want, we know
 * that it hasn't changed since the have state (i.e. unpredictable
 * changes are never deferred).
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
	SKIPFWD, /* forward condition to subsequent skip instruction */
};

static unsigned char skipstate = NORMAL;

/* acstate templates */
static const struct expr zero = { RCONST | 0, "" };
static const struct expr random = { RANDOM, "" };
static const struct expr invalid = { INVALID, "" };

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

extern void
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
	deferred[ndefer].e = e == NULL ? invalid : *e;
	ndefer++;
}

/* sequences for findseq */
static const unsigned short seq1preservel[][3] = {
	00000, CLA,			0,
	00001, CLA | IAC,		0,
	07777, STA,			0,
	00000, 0,			0,
};

static const unsigned short seq1clearl[][3] = {
	00000, CLA | CLL,		0,
	00001, CLA | CLL | IAC,		0,
	00002, CLA | STL | RTL,		0,
	00003, CLA | STL | IAC | RAL,	0,
	00004, CLA | CLL | IAC | RTL,	0,
	00006, CLA | STL | IAC | RTL,	0,
	02000, CLA | STL | RTR,		0,
	04000, CLA | STL | RAR,		0,
	06000, CLA | STL | IAC | RTR,	0,
	07777, STA | CLL,		0,
	00000, 0,			0,
};

static const unsigned short seq1setl[][3] = {
	00000, CLA | STL,		0,
	00001, CLA | STL | IAC,		0,
	04000, CLA | STL | IAC | RAR,	0,
	03777, STA | CLL | RAR,		0,
	05777, STA | CLL | RTR,		0,
	07775, STA | CLL | RTL,		0,
	07776, STA | CLL | RAL,		0,
	07777, STA | STL,		0,
	00000, 0,			0,
};

static const unsigned short seq2preservel[][3] = {
	00002, CLA | IAC,		IAC,
	00003, CLA | IAC | RAR,		IAC | RAL,
	00004, CLA | RTR,		IAC | RTL,
	00006, CLA | RTR,		STL | IAC | RTL,
	02000, CLA | RTL,		STL | RTR,
	03777, STA | RAL,		CLL | RAR,
	04000, CLA | RAL,		STL | RAR,
	06000, CLA | RTL,		STL | IAC | RTR,
	06777, STA | RTL,		CLL | RTR,
	07775, STA | RTR,		CLL | RTL,
	07776, STA | RAR,		CLL | RAL,
	00000, 0,			0,
};

static const unsigned short dummyseq[1][3] = { 0, 0, 0 };
#define seq2clearl dummyseq
#define seq2setl dummyseq

/*
 * Find a sequence of up to 2 OPR instructions that produce lac
 * in L:AC.  If such a sequence is found, defer it and return 1.
 * Otherwise, return 0.
 */
static int
findseq(const unsigned short seq[][3], int lac)
{
	int i;

	for (i = 0; seq[i][1] != 0; i++)
		if (seq[i][0] == lac) {
			defer(seq[i][1], NULL);
			if (seq[i][2] != 0)
				defer(seq[i][2], NULL);

			return (1);
		}

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
	int wantac, haveac, acknown, preservel = 0, flipl = 0, clearl = 0, setl = 0;
	struct expr e = { 0, "" };

	/* discard deferred instructions */
	ndefer = 0;

	wantac = want.lac & 07777;
	haveac = have.lac & 07777;
	acknown = have.known & ACKNOWN;

	/* determine possible strategies */
	if (want.known & LANY) {
		preservel = 1;
		flipl = 1;
		clearl = 1;
		setl = 1;
	} else if (want.known & LKNOWN) {
		if (want.lac & 010000)
			setl = 1;
		else
			clearl = 1;

		if (have.known & LKNOWN) {
			if ((have.lac & 010000) == (want.lac & 010000))
				preservel = 1;
			else
				flipl = 1;
		}
	} else
		preservel = 1;

	/* AC already set up? */
	if (~want.known & ACKNOWN || acknown && wantac == haveac) {
		/* need to set up L? */
		if (!preservel)
			defer(want.lac & 010000 ? STL : CLL, NULL);

		return;
	}

	/* strategy 1--3: 1 instruction OPR sequences */
	if (clearl && findseq(seq1clearl, wantac)) {
		want.known |= LKNOWN;
		want.lac &= ~010000;
		return;
	}

	if (setl && findseq(seq1setl, wantac)) {
		want.known |= LKNOWN;
		want.lac |= 010000;
		return;
	}

	if (!setl && !clearl && preservel && findseq(seq1preservel, wantac))
		return;

	/* strategy 4--6: 2 instruction OPR sequences */
	if (clearl && findseq(seq2clearl, wantac)) {
		want.known |= LKNOWN;
		want.lac &= ~010000;
		return;
	}

	if (setl && findseq(seq2setl, wantac)) {
		want.known |= LKNOWN;
		want.lac |= 010000;
		return;
	}

	if (!setl && !clearl && preservel && findseq(seq2preservel, wantac))
		return;

	/* strategy 7--9: 1 instruction TAD/AND sequences */
	if (acknown && preservel && haveac <= wantac) {
		e.value = RCONST | wantac - haveac;
		defer(TAD, &e);
		want.lac = wantac | have.lac & 010000;
		return;
	}

	if (acknown && flipl && haveac > wantac) {
		e.value = RCONST | wantac - haveac;
		defer(TAD, &e);
		want.lac = wantac | ~have.lac & 010000;
		return;
	}

	if (acknown && preservel && (~haveac & wantac) == 0) {
		e.value = RCONST | wantac;
		defer(AND, &e);
		want.lac = wantac | have.lac & 010000;
		return;
	}

	/* strategy 10: just do whatever is needed */
	if (clearl)
		defer(CLA | CLL, NULL);
	else if (setl)
		defer(CLA | STL, NULL);
	else /* preservel */
		defer(CLA, NULL);

	e.value = RCONST | wantac;
	defer(TAD, &e);
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
	 * must_emit & 2, we also need to set acstate to random.
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
		if (want.known & ACKNOWN) {
			if (isconst(v)) {
				want.lac = 017777 & want.lac + val(v);

				/* must emit this if L is unknown and needs to be flipped. */
				if (~want.known & LKNOWN && ~want.known & LANY
				    && (want.lac & 07777) + val(v) > 07777)
					must_emit |= 1;

				break;

				/* special case: use TAD to load a value */
			} else if ((want.lac & 007777) == 0) {
				want.known &= ~ACKNOWN;
				must_emit |= 1;
				acstate = *e;
				break;
			}
		}

		/* general case: nothing can be assumed */
		want.known &= ~LKNOWN & ~ACKNOWN;
		must_emit |= 3;

		break;

	case ISZ:
		skipstate = SKIPABLE;
		must_emit |= 1;

		if (inac(e->value))
			must_emit |= 2;

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

		case BSW:
			if (will.known & ACKNOWN)
				will.lac = will.lac & 010000 |
				    will.lac << 6 & 007700 | will.lac >> 6 & 000077;
			else {
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

			if (~will.known & ACKNOWN) {
				skipstate = SKIPABLE;
			} else if (will.lac & 004000) {
				skipstate = DOSKIP;
			}

			break;

		case SZA:
			if (skipstate == DOSKIP)
				break;

			if (~will.known & ACKNOWN) {
				skipstate = SKIPABLE;
			} else if ((will.lac & 007777) == 0) {
				skipstate = DOSKIP;
			}

			break;

		case SNL:
			if (skipstate == DOSKIP)
				break;

			if (~will.known & LKNOWN) {
				skipstate = SKIPABLE;
			} else if (will.lac & 010000) {
				skipstate = DOSKIP;
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
	}}

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
	int affects_lac = 0, ac_is_clear, o;

	ac_is_clear = want.known & ACKNOWN && (want.lac & 07777) == 0;

	/*
	 * check if this is a skip + IAC sequence and forward the
	 * the condition.
	 */
	if ((op & ~00200) == IAC && (ac_is_clear || op & 00200)) {
		skipstate = SKIPFWD;
		acstate = random;
		want.known = 0;
		defer(op, e);
		return;
	}

	skipstate = NORMAL;

	switch (op & 07000) {
	case TAD:
	case AND:
		affects_lac = 1;
		break;

	case ISZ:
		skipstate = SKIPABLE;
		break;

	case DCA:
		if (!ac_is_clear)
			affects_lac = 1;

		break;

	case JMP:
		break;

	case OPR:
		o = op;

		for (;;) switch (peelopr(&o)) {
		case NOP:
			goto peeled;

		case BSW:
		case CLA:
			if (!ac_is_clear)
				affects_lac = 1;

			break;

		case CLL:
			if (~want.known & LANY && (~want.known & LKNOWN || (want.lac & 010000) != 0))
				affects_lac = 1;

			break;

		case CML:
			if (~want.known & LANY)
				affects_lac = 1;

			break;

		case RAR:
		case RAL:
		case RTR:
		case RTL:
		case IAC:
		case CMA:
			affects_lac = 1;
			break;

		case SMA:
		case SZA:
		case SNL:
		case SKP:
			affects_lac = 1;
			skipstate = SKIPABLE;
			break;

		default:
			fatal(NULL, "unregonised OPR instruction: %04o", o & 07777);
		}

	peeled:	break;
	}

	if (affects_lac) {
		acstate = random;
		want.known = 0;
	}

	undefer();
	emitisn(op, e);
}

extern void
iselrst(void)
{
	acstate = zero;
	ndefer = 0;
	want.lac = 0;
	want.known = LANY | ACKNOWN;
	have = want;
	skipstate = NORMAL;
}

extern void
iselacrnd(void)
{
	acstate = random;
	want.known = LANY;
	undefer();
}

extern void
lany(void)
{
	want.known |= LANY;
	if (skipstate == NORMAL)
		fold();
}

extern void
isel(int op, const struct expr *e)
{
	switch (skipstate) {
	case DOSKIP:
		/* discard skip and current instruction if possible */
		if (ndefer != 0) {
			ndefer--;
			skipstate = NORMAL;
			break;
		}

		/*
		 * if the skip instruction has already been emitted,
		 * treat op like a skipable instruction.
		 */

		/* FALLTHROUGH */

	case SKIPABLE:
		skipsel(op, e);
		break;

	case SKIPFWD:
		/* discard SZA/SNA if possible */
		if (ndefer < 2) {
			skipstate = NORMAL;
			goto normal;
		}


		switch (op) {
		case SZA | CLA:
			if (ndefer < 1)
				goto normal;

			break;

		case SNA | CLA:
			/* can only toggle if a skip instruction was deferred */
			if (ndefer < 2 || (deferred[ndefer - 2].op & OPR2) != OPR2)
				goto normal;

			/* toggle skip condition */
			deferred[ndefer - 2].op ^= 00010;
			break;

		default:
			goto normal;
		}

		/* record effect of CLA */
		acstate = zero;
		want.known |= ACKNOWN;
		want.lac &= ~07777;

		/* discard (CLA) IAC and current skip */
		ndefer--;
		break;

		/* can't forward conditional */
	normal:
		skipstate = NORMAL;
		/* FALLTHROUGH */

	case NORMAL:
		normalsel(op, e);
		break;
	}
}
