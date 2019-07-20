/* main.c -- program entry point */

#include <stdio.h>
#include <stdlib.h>

#include "param.h"
#include "pdp8.h"
#include "asm.h"
#include "data.h"
#include "error.h"
#include "name.h"
#include "parser.h"

/*
 * Standard library symbol names.  Each name must occur in the
 * B runtime code and is mapped to a L#### label in main.  Names
 * are truncated to 6 characters in the runtime code to fit PAL8
 * restrictions.
 *
 * TODO: only do the mapping if the symbol was not redefined by the
 * program.
 */
enum { NSTDLIB = 4 };
static const char stdlib[NSTDLIB][8] = {
	"EXIT", "GETCHAR", "PUTCHAR", "SENSE",
};

extern int
main(void)
{
	struct expr *e;
	size_t i;

	asmfile = stdout;

	yyparse();
	dumpdata();

	/* tell the B runtime where MAIN is */
	e = define("MAIN");
	label("MAIN=");
	emitl(e);

	/* translate library labels to L### labels */
	for (i = 0; i < NSTDLIB; i++) {
		e = define(stdlib[i]);
		setlabel(e);
		instr("%.6s", stdlib[i]);
	}

	instr("$");
	endline();

	if (warncnt > 0)
		fprintf(stderr, "%d warnings\n", warncnt);

	if (errcnt > 0) {
		fprintf(stderr, "%d errors\n", errcnt);
		return (EXIT_FAILURE);
	}

	return (EXIT_SUCCESS);
}
