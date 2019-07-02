/* param.h -- program parameters */

/* zero page usage
 *
 * 0000--0007 interrupt handler
 * 0010--0017 indexed memory locations
 * 0020--0027 runtime registers
 * 0030--0177 scratch registers
 *
 * all scratch registers must be preserved by the callee.
 *
 * the runtime registers are used as follows:
 * 0010 pointer to the ENTER routine
 * 0011 pointer to the LEAVE routine
 * 0012 pointer to the MUL   routine
 * 0013 pointer to the DIV   routine
 * 0014 pointer to the MOD   routine
 * 0015--0017 runtime registers
 */
enum {
	NZEROPAGE = 00200,			/* number of storage locations in the zero page */
	MINSCRATCH = 00030,			/* the first scratch register */
	NSCRATCH = NZEROPAGE - MINSCRATCH,	/* number of scratch registers */
};

/* table sizes */
enum {
	DEFNSIZ = 00400,			/* definition table size */
	DECLSIZ = 00040,			/* declaration table size */
};

/* various parameters */
enum {
	MAXERRORS = 10,				/* number of errors before the compiler gives up */
};
