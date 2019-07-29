/* pdp8.c -- instruction selection */

#include <stdio.h>
#include <string.h>

#include "param.h"
#include "pdp8.h"
#include "asm.h"
#include "codegen.h"
#include "error.h"
#include "data.h"
#include "name.h"

extern struct expr
r2lval(const struct expr *e)
{
	struct expr r = *e;
	int v = e->value;

	if (isvalid(v) && isrval(v))
		r.value = v | LMASK;
	else
		r.value = NORVAL;

	return (r);
}

extern struct expr
l2rval(const struct expr *e)
{
	struct expr r = *e;
	int v = e->value;

	if (isvalid(v) && islval(v))
		r.value = v & ~LMASK;
	else
		r.value = NOLVAL;

	return (r);
}

/* stubs */
extern void and(const struct expr *e) { emitand(e); }
extern void tad(const struct expr *e) { emittad(e); }
extern void isz(const struct expr *e) { emitisz(e); }
extern void dca(const struct expr *e) { emitdca(e); }
extern void jms(const struct expr *e) { emitjms(e); }
extern void jmp(const struct expr *e) { emitjmp(e); }

extern void
lda(const struct expr *e)
{
	opr(CLA);
	tad(e);
}

extern void
opr(int op)
{
	emitopr(op);
}

extern void
push(struct expr *e)
{
	emitpush(e);
	dca(e);
}

extern void
pop(struct expr *e)
{
	emitpop(e);
}

extern void
clearac(void)
{
	;
}

extern void
catchup(void)
{
	;
}
