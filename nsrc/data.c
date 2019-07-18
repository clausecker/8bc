/* data.c -- data area management */

#include <stddef.h>
#include <stdio.h>

#include "param.h"
#include "pdp8.h"
#include "asm.h"
#include "data.h"
#include "error.h"

/* the data area and the next free spot in it */
static unsigned short data[DATASIZ];
static unsigned short here = 0;

extern void
todata(int c)
{
	if (here >= DATASIZ)
		fatal(NULL, "data area full");

	data[here++] = c & 07777;
}

extern void
newdata(struct expr *e)
{
	e->value = here | LDATA;
}

extern void
literal(struct expr *e, int c)
{
	int i;

	c &= 07777;

	for (i = 0; i < here; i++)
		if (data[i] == c)
			goto found;

	/* not found */
	todata(c);

found:	e->value = i | LDATA;
}

extern void
dumpdata(void)
{
	size_t i;

	/* don't dump data if there is none */
	if (here == 0)
		return;

	label("DATA,");

	for (i = 0; i < here; i++)
		emitc(data[i]);

	blank();
}
