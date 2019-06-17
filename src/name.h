/* name tables */

enum {
	/* storage dispositions */
	EMPTY	= 000000, /* empty symbol table entry */
	AC	= 000001, /* value is stored in AC */
	TOKEN	= 000002, /* token returned by yylex() */

	LVALUE	= 010000, /* address of value is stored in register */
	RVALUE	= 020000, /* value is stored in register */
	LABEL	= 030000, /* internal or external at label L#### */
	UNDECL	= 040000, /* undeclared at label L#### */
	UNDEFN  = 040000, /* undefined at label L#### */
	CONST	= 050000, /* constant value */
	INDEX   = 060000, /* index into the declaration table */
	DSPMASK	= 070000, /* disposition mask */

	MAXNAME = 8,      /* maximal name length */
	MAXDECL = 00200,  /* maximum number of declarations */
	MAXDEFN = 01000,  /* maximum number of definitions */
};

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
