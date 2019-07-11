/* main.c -- program entry point */

#include <stdio.h>
#include <stdlib.h>

#include "param.h"
#include "asm.h"
#include "error.h"
#include "parser.h"

extern int
main(void)
{
	asmfile = stdout;

	yyparse();
	instr("$\n");

	if (warncnt > 0)
		fprintf(stderr, "%d warnings\n", warncnt);

	if (errcnt > 0) {
		fprintf(stderr, "%d errors\n", errcnt);
		return (EXIT_FAILURE);
	}

	return (EXIT_SUCCESS);
}
