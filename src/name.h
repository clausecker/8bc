/* name tables */

enum {
	/* storage dispositions */
	CONST	= 0000000, /* constant value */
	UNDEF	= 0100000, /* must not occur */
	RVALUE	= 0010000, /* value is stored in zero page */
	LVALUE	= 0110000, /* address of value is stored in zero page */
	RLABEL	= 0020000, /* pointer to label L#### (does not occur) */
	LLABEL  = 0120000, /* label L#### */
	RUNDECL = 0030000, /* pointer to undeclared label (does not occur) */
	LUNDECL = 0130000, /* undeclared label */
	RSTACK  = 0040000, /* rvalue on the stack */
	LSTACK	= 0140000, /* lvalue on the stack */
	RAUTO   = 0050000, /* pointer to automatic variable area */
	LAUTO   = 0150000, /* variable in automatic variable area */
	RARG    = 0060000, /* pointer to function argument */
	LARG    = 0160000, /* function argument */

	DSPMASK	= 0170000, /* disposition mask */
	TYPMASK = 0070000, /* disposition mask with the MSB clear */
	LMASK   = 0100000, /* lvalue mask */

	MAXNAME = 8,      /* maximal name length */
	MAXDECL = 00200,  /* maximum number of declarations */
	MAXDEFN = 01000,  /* maximum number of definitions */
};

/* convenience macros */
#define dsp(a) ((a) & DSPMASK)
#define val(a) ((a) & ~DSPMASK)
#define onstack(a) (((a) & TYPMASK) == RSTACK)
#define islval(a) ((a) & LMASK)
#define bothconst(a, b) (dsp(a) == CONST && dsp(b) == CONST)

/* for printf */
#define NAMEFMT "%.8s"

/*
 * declaration table
 *
 * this table stores names declared inside functions.  There are three
 * storage classes:
 *
 *  - external names refer to an externally defined object.  When such
 *    a name is declared, its value is taken from the corresponding
 *    entry in the definition table.  If the name is not known in that
 *    table, an entry is created.  The disposition is LABEL.
 *
 *  - automatic names refer to an automatic variable.  When such a name
 *    declared, space for the variable is allocated in the activation
 *    record and its lvalue is stored in a register.  The disposition is
 *    LVALUE.
 *
 *  - internal names refer to labels.  When a label is set, it is
 *    declared if necessary and its disposition is set to LABEL.  If an
 *    uneclared name is observed, it is believed to be of internal
 *    storage class and its disposition is set to UNDECL.
 *
 * the amount of declarations in this table is stored in ndecls.
 * Whenever a scope is left, all declarations in that scope are removed
 * from the table by decreasing ndecls.
 *
 * the expr structure is also used as YYSTYPE.  Other storage
 * dispositions may occur in this case.
 */
#define YYSTYPE struct expr
extern YYSTYPE yylval;
extern unsigned short ndecls;
extern struct expr {
	char name[MAXNAME];
	unsigned short value;
} decls[MAXDECL];

/*
 * definition table
 *
 * this table is like the declaration table, but it stores the label
 * numbers for global definitions.  When a name has been referred to
 * but has not yet been defined, it is entered as UNDEFN into the table.
 * Once a definition has been seen, this is changed to LABEL.  This way,
 * we can both catch undefined external names and doubly defined names.
 *
 * the amount of definitions in this table is stored in ndefns.  This
 * number never goes down.
 */
extern unsigned short ndefns;
extern struct expr defns[MAXDEFN];

extern int define(struct expr *);
extern int declare(struct expr *);

/*
 * The next label to allocate.
 */
extern unsigned short labelno;
