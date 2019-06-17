/* name tables */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "name.h"
#include "y.tab.h"

unsigned short ndefns = 0, ndecls = 0, labelno = 0;
struct expr decls[MAXDECL], defns[MAXDEFN];

/*
 * Add the given name to the given name table.  If the table would
 * hold more than maxname entries afterwards, report that tablename
 * is exhausted and abort.  Return the number of the table entry
 * If the name is already present, instead copy the
 * table entry for it to expr and return the index.  Assign the new
 * number of entries to *nname.
 */
static int
addname(struct expr *names, unsigned short *nnames, int maxname,
    const char *tablename, const struct expr *expr)
{
	int i;

	for (i = *nnames - 1; i >= 0; i--)
		if (memcmp(names[i].name, expr->name, MAXNAME) == 0)
			goto found;

	/* not found */
	if (*nnames >= maxname) {
		fprintf(stderr, "%s table exhausted\n", tablename);
		return (EXIT_FAILURE);
	}

	names[*nnames] = *expr;

	return ((*nnames)++);

found:	return (i);
}

/*
 * Add the name referred to by expr to the definition table.  If it does
 * not exist, allocate a new label for it and set it to UNDEFN.  Always
 * copy the content of the defn table to expr.
 */
extern int
define(struct expr *expr) {
	int i;

	expr->value = EMPTY;
	i = addname(defns, &ndefns, MAXDEFN, "definition", expr);
	if (defns[i].value == EMPTY)
		defns[i].value = labelno++ | UNDEFN;

	expr->value = defns[i].value;

	return (i);
}

extern int
declare(struct expr *expr) {
	return (addname(decls, &ndecls, MAXDECL, "declaration", expr));
}
