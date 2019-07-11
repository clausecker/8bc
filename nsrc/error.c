/* error.h -- error handling */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "param.h"
#include "pdp8.h"
#include "error.h"
#include "parser.h"

short errcnt = 0;

static void
verror(const char *name, const char *fmt, va_list ap)
{
	fprintf(stderr, "%5d " NAMEFMTF " ", lineno, name == NULL ? "" : name);
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
}

extern void
warn(const char *name, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	verror(name, fmt, ap);
	va_end(ap);
}

extern void
error(const char *name, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	verror(name, fmt, ap);
	va_end(ap);

	if (++errcnt >= MAXERRORS)
		fatal(NULL, "too many errors");
}

extern void
fatal(const char *name, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	verror(name, fmt, ap);
	va_end(ap);

	exit(EXIT_FAILURE);
}

extern void
yyerror(const char *msg)
{
	error(NULL, "%s", msg);
}
