/*
 * A struct expr stores the value of an expression.  A name can be
 * associated with the expression in which case it is stored in name.
 * value describes the storage class and value (for RCONST) or
 * location (for all other classes) of the expression.
 *
 * A name is up to MAXNAME characters long.  NAMEFMT can be used to
 * print it.
 */
enum { MAXNAME = 8 };
#define NAMEFMT "%.8s"
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
	RUND   = 0030000, /* undefined/undeclared RLABEL */
	LUND   = 0130000, /* undefined/undeclared LLABEL */
	RSTACK = 0040000, /* stack register is value */
	LSTACK = 0140000, /* stack register points to value */
	RAUTO  = 0050000, /* pointer to automatic variable area */
	LAUTO  = 0150000, /* variable in automatic variable area */
	RARG   = 0060000, /* pointer to argument */
	LARG   = 0160000, /* argument */
	RINVAL = 0070000, /* invalid storage class */
	LINVAL = 0170000, /* invalid storage class */

	CMASK  = 0170000, /* storage class mask */
	LMASK  = 0100000, /* lvalue mask */
};

/*
 * utility macros for dealing with storage classes.
 */
#define val(x) ((x) & ~CMASK)
#define class(x) ((x) & CMASK)
#define isconst(x) (class(x) == RCONST)
#define islabel(x) (((x) & 0060000) == RLABEL)

/*
 * There are two name tables in this compiler.  The defns table
 * stores (global) definitions, while the decls table store local
 * declarations.  Each entry in the defns table has class LLABEL
 * while The following functions are available:
 *
 * expr = define(name)
 *     Look up name in the definition table and return a pointer to
 *     the entry found.  If no entry was found, a new one is allocated
 *     and set to RUNDECL and a new label number.
 *
 * expr = lookup(name)
 *     Look up name in the declaration table.  If name is found, return
 *     a pointer to its entry.  If multiple entries are found, return
 *     the last one.  If no entry is found, return NULL.
 *
 * expr = declare(expr)
 *     Enter expr into the declaration table and return a pointer to the
 *     new entry.  Does not check if the entry already exists.
 *
 * scope = beginscope()
 *     Begin a new scope in the declaration table.  Return the number of
 *     the scope.
 *
 * endscope(scope)
 *     End scope in the declaration table.  All entries entered after
 *     the corresponding beginscope() call are removed.
 */
extern struct expr *define(const char name[MAXNAME]);
extern struct expr *lookup(const char name[MAXNAME]);
extern struct expr *declare(struct expr *);
extern int beginscope(void);
extern void endscope(int);

/* TODO */
extern void newlabel(struct expr *);
