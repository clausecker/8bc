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

extern int
main(void)
{
	struct expr *main;

	asmfile = stdout;

	yyparse();
	dumpdata();
	main = define("MAIN");
	label("MAIN=");
	emitl(main);
	instr("$\n");

	if (warncnt > 0)
		fprintf(stderr, "%d warnings\n", warncnt);

	if (errcnt > 0) {
		fprintf(stderr, "%d errors\n", errcnt);
		return (EXIT_FAILURE);
	}

	return (EXIT_SUCCESS);
}
