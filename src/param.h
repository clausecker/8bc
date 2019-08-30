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
 * 0020 pointer to the ENTER routine
 * 0021 pointer to the LEAVE routine
 * 0022 pointer to the MUL   routine
 * 0023 pointer to the DIV   routine
 * 0024 pointer to the MOD   routine
 * 0025--0027 runtime registers
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
	DATASIZ = 01000,			/* data area size */
	ARGSIZ  = 00040,			/* maximum number of arguments in parser */
};

/* various parameters */
enum {
	MAXERRORS = 10,				/* number of errors before the compiler gives up */
	MAXNAME = 8,				/* maximum name size */
};

#define NAMEFMT "%.8s"				/* format string to print a name */
#define NAMEFMTF "%-8.8s"			/* same, but with a fixed field size */
