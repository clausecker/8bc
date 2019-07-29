/* name.c -- name tables */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "asm.h"
#include "error.h"
#include "param.h"
#include "pdp8.h"
#include "name.h"

/* definition and declaration tables */
static struct expr defns[DEFNSIZ], decls[DECLSIZ];
static unsigned short ndecl = 0, ndefn = 0;

/* the next label number to use */
static short labelno = 0;

extern struct expr *
define(const char name[MAXNAME])
{
	int i;

	for (i = 0; i < ndefn; i++)
		if (strncmp(defns[i].name, name, MAXNAME) == 0)
			goto found;

	/* not found */
	if (i >= DEFNSIZ)
		fatal(name, "defn table full");

	strncpy(defns[i].name, name, MAXNAME);
	newlabel(defns + i);
	ndefn++;

found:	return (defns + i);
}

extern struct expr *
lookup(const char name[MAXNAME])
{
	int i;

	for (i = ndecl - 1; i >= 0; i--)
		if (strncmp(decls[i].name, name, MAXNAME) == 0)
			return (decls + i);

	/* not found */
	return (NULL);
}

/* TODO: add a mechanism to detect redeclarations */
extern struct expr *
declare(struct expr *e)
{
	if (ndecl >= DECLSIZ)
		fatal(e->name, "decl table full");

	decls[ndecl] = *e;

	return (decls + ndecl++);
}

extern int
beginscope(void)
{
	return (ndecl);
}

extern void
endscope(int scope)
{
	if (scope < 0 || scope > ndecl)
		fatal(NULL, "invalid scope");


	ndecl = scope;
}

extern void
newlabel(struct expr *e)
{
	e->value = LLABEL | labelno++;
	if (labelno > 07777)
		fatal(e->name, "too many labels");
}

/*
 * Place a label.  If the label is not actually a label, fail
 * compilation.  Suffix the label with suffix.
 */
static void
placelabel(const struct expr *e, int suffix)
{
	if (rclass(e->value) != RLABEL)
		fatal(e->name, "not a label");

	label("L%04o%c", val(e->value), suffix);
}

extern void
putlabel(const struct expr *e)
{
	catchup();
	placelabel(e, ',');
}

extern void
setlabel(const struct expr *e)
{
	placelabel(e, '=');
}
