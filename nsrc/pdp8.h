/*
 * A struct expr stores the value of an expression.  A name can be
 * associated with the expression in which case it is stored in name.
 * value describes the storage class and value (for RCONST) or
 * location (for all other classes) of the expression.
 *
 * A name is up to MAXNAME characters long.  NAMEFMT can be used to
 * print it.
 */
struct expr {
	unsigned short value;
	char name[MAXNAME];
};

/*
 * Each object resides in a storage class.  Each storage class exists
 * as an rvalue and an lvalue version.  The lvalue version of a storage
 * class interpretes the rvalue version as an address and refers to the
 * memory it points to.  The difference between lvalue and rvalue is
 * discovered by checking bit 15.
 */
enum {
	RCONST = 0000000, /* constant */
	LCONST = 0100000, /* absolute address (like RVALUE) */
	RVALUE = 0010000, /* zero page register is value */
	LVALUE = 0110000, /* zero page register points to value */
	RLABEL = 0020000, /* label is value */
	LLABEL = 0120000, /* label points to value */
	RDATA  = 0030000, /* pointer to data area */
	LDATA  = 0130000, /* value in data area */
	RSTACK = 0040000, /* stack register is value */
	LSTACK = 0140000, /* stack register points to value */
	RAUTO  = 0050000, /* pointer to automatic variable area */
	LAUTO  = 0150000, /* variable in automatic variable area */
	RPARAM = 0060000, /* pointer to parameter */
	LPARAM = 0160000, /* parameter */
	SPECIAL= 0070000, /* special class */
	INVALID= 0170000, /* invalid storage class */

	CMASK  = 0170000, /* storage class mask */
	LMASK  = 0100000, /* lvalue mask */
};

/*
 * utility macros for dealing with storage classes.
 */
#define val(x) ((x) & ~CMASK)
#define class(x) ((x) & CMASK)
#define rclass(x) ((x) & CMASK & ~LMASK)
#define isconst(x) (class(x) == RCONST)
#define islval(x) ((x) & LMASK)
#define isrval(x) (!islval(x))
#define isvalid(x) (rclass(x) != SPECIAL)
#define onstack(x) (rclass(x) == RSTACK)

/*
 * the storage class INVALID is used to mark various non-expressions.
 * These are listed here.
 */
enum {
	TOKEN   = INVALID | 0000001, /* token returned by yylex */
	EXPIRED = INVALID | 0000002, /* expired stack register */
	NORVAL  = INVALID | 0000003, /* not an rvalue */
	NOLVAL  = INVALID | 0000004, /* not an lvalue */
};

/*
 * Generate code for the specified PDP-8 instruction.  The function
 * argument is provided as an argument to the instruction with an
 * appropriate addressing mode.  lda() is a convenience function
 * that clears AC and then adds the desired value.
 *
 * It is not guaranteed that the desired instruction is actually
 * emitted as the optimiser may replace it with another
 * instruction.
 */
extern void and(const struct expr *);
extern void tad(const struct expr *);
extern void isz(const struct expr *);
extern void dca(const struct expr *);
extern void jms(const struct expr *);
extern void jmp(const struct expr *);
extern void lda(const struct expr *);

/*
 * Generate the given microcoded PDP-8 instruction.  Use the provided
 * macros to build microcoded instructions.  As with a real PDP-8,
 * microinstructions may not be mixed across groups except for CLA.
 * Group 3 instructions as well as OSR and HLT are not supported and
 * must be manually emitted if needed.
 */
enum {
	/* group 1 */
	OPR1 =        07000,
	CLA  = OPR1 | 00200, /* clear AC */
	CLL  = OPR1 | 00100, /* clear L */
	CMA  = OPR1 | 00040, /* complement AC */
	CML  = OPR1 | 00020, /* complement L */
	RAR  = OPR1 | 00010, /* rotate AC right */
	RAL  = OPR1 | 00004, /* rotate AC left */
	BSW  = OPR1 | 00002, /* byte swap (not supported) / rotate twice */
	IAC  = OPR1 | 00001, /* increment AC */

	NOP  = OPR1,
	RTR  = RAR  | BSW,   /* rotate twice right */
	RTL  = RAL  | BSW,   /* rotate twice left */
	STA  = CLA  | CMA,   /* set AC */
	STL  = CLL  | CML,   /* set L */
	CIA  = CMA  | IAC,   /* complement and increment AC (negate AC) */
	GLK  = CLA  | RAL,   /* get link */

	/* group 2 */
	OPR2 =        07400,
	SMA  = OPR2 | 00100, /* skip on minus AC */
	SZA  = OPR2 | 00040, /* skip on zero AC */
	SNL  = OPR2 | 00020, /* skip on non-zero L */
	SKP  = OPR2 | 00010, /* reverse skip condition */

	SPA  = SKP  | SMA,   /* skip on positive AC */
	SNA  = SKP  | SZA,   /* skip on non-zero AC */
	SZL  = SKP  | SNL,   /* skip on zero L */

	/* privileged instructions (not supported) */
	OSR  = OPR2 | 00004, /* or switch register */
	HLT  = OPR2 | 00002, /* halt */

	/* group 3 (not supported) */
	OPR3 = OPR2 | 00001
};

extern void opr(int);

/*
 * lvalues and rvalues.
 *
 * expr = r2lval(expr)
 *     Interprete an rvalue as an lvalue, effectively dereferencing expr.
 *     expr must have storage class RCONST, RVALUE, RLABEL, RDATA, RSTACK,
 *     RAUTO, or RPARAM.  If the class is invalid, the result's value is
 *     NORVAL.
 *
 * expr = l2rval(expr)
 *     Interprete an lvalue as an rvalue, effectively taking the address
 *     of expr.  expr must have storage class LCONST, LVALUE, LLABEL,
 *     LDATA, LSTACK, LAUTO, or LPARAM.  If the class is invalid, the
 *     result's value is NOLVAL.
 */
extern struct expr r2lval(const struct expr *);
extern struct expr l2rval(const struct expr *);

/*
 * Call frame management.
 *
 * push(expr)
 *     Generate an expression referring to the content of AC.  This
 *     might involve allocating a new stack register for it.  The
 *     content of AC is undefined after this instruction.
 *
 * pop(expr)
 *     Mark expr as no longer needed and possibly free the stack
 *     register allocated for it.  Only the most recently
 *     allocated scratch register can be deallocated.  expr is
 *     overwritten with EXPIRED to prevent accidental reuse.  If expr
 *     is not on the stack, this does nothing.  This has no effect on
 *     the content of AC.
 */
extern void push(struct expr *);
extern void pop(struct expr *);

/*
 * State management.
 *
 * acclear()
 *     Notify the optimiser that you expect the AC to be clear at this
 *     point in time.
 *
 * catchup()
 *     Tell the optimiser to emit instructions such that the actual
 *     state of the machine equals the simulated state of the machine.
 */
extern void acclear(void);
extern void catchup(void);
