/* pdp8.c -- PDP-8 code generation */

#include <stdio.h>
#include <string.h>

#include "param.h"
#include "asm.h"
#include "error.h"
#include "pdp8.h"
#include "name.h"

/*
 * Labels related to the current function.
 *
 * framelabel
 *     beginning of the frame record
 *
 * stacklabel
 *     first register on the stack
 *
 * autolabel
 *     beginning of the automatic variable area
 */
static struct expr framelabel = { 0, "(frame)" };
static struct expr stacklabel = { 0, "(stack)" };
static struct expr autolabel = { 0, "(auto)" };



/*
 * Generate a string representation of the address of e as needed for
 * emitl.  e must be of type LCONST, RVALUE, LLABEL, LUND, RSTACK,
 * RAUTO, or RARG.  The returned string is placed in a static buffer.
 */
static const char *
lstr(const struct expr *e)
{
	static char buf[14];
	int v = e->value;

	switch (class(v)) {
	case LCONST:
	case RVALUE:
		sprintf(buf, "%04o", val(v));
		break;

	case LLABEL:
	case LUND:
		sprintf(buf, "L%04o", val(v));
		break;

	case RSTACK:
		sprintf(buf, "L%04o+%03o", val(stacklabel.value), val(v));
		break;

	case RAUTO:
		sprintf(buf, "L%04o+%03o", val(autolabel.value), val(v));
		break;

	case RARG:
		sprintf(buf, "L%04o+%03o", val(framelabel.value), val(v) + 2);
		break;

	default:
		fatal(e->name, "invalid arg to %s: %06o", __func__, val(v));
	}

	return (buf);
}

extern void
emitl(const struct expr *e)
{
	return (instr(lstr(e)));
}

extern void
emitr(const struct expr *e)
{
	struct expr le;

	switch (class(e->value)) {
	case RCONST:
	case RLABEL:
	case RUND:
		le = r2lval(e);

		return (instr(lstr(&le)));

	default:
		fatal(e->name, "invalid arg to %s: %06o", __func__, val(e->value));
	}
}

/*
 * Turn expr into a string suitable as an argument to a PDP-8
 * instruction.  If needed, spill the argument into a zero page
 * register.  The resulting string is stored in a static buffer
 * which is reused by the next call to this function.
 */
static const char *
lit(struct expr *e)
{
	static char buf[16];
	int v = e->value;
	const char *prefix = "";

	switch (class(v)) {
	case LLABEL:
	case LUND:
	case RLABEL:
	case RUND:
	case LCONST:
	case RCONST:
		/* TODO: spill */
		fatal(e->name, "spill not implemented");

	case LVALUE:
	case LSTACK:
	case LAUTO:
	case LARG:
		prefix = "I ";
		break;

	case RVALUE:
	case RSTACK:
	case RAUTO:
	case RARG:
		break;

	default:
		fatal(e->name, "invalid arg to %s: %06o", __func__, val(v));
	}

	sprintf(buf, "%s%s", prefix, lstr(e));

	return (buf);
}
