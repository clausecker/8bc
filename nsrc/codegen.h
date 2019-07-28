/*
 * Emit the specified PDP-8 instruction.  The function argument is
 * provided as an argument to the instruction with an appropriate
 * addressing mode.
 */
extern void emitand(const struct expr *);
extern void emittad(const struct expr *);
extern void emitisz(const struct expr *);
extern void emitdca(const struct expr *);
extern void emitjms(const struct expr *);
extern void emitjmp(const struct expr *);
extern void emitopr(int);

/*
 * Emitting literal values.
 *
 * emitr(expr)
 *     Emits the value of expr into the instruction stream.  expr must
 *     be of type RCONST, RLABEL, RDATA, RAUTO, or RPARAM.
 *
 * emitl(expr)
 *     Emits the address of expr into the instruction stream.  expr can
 *     be of type LCONST, RVALUE, LLABEL, LDATA, RSTACK, LAUTO, or
 *     LPARAM.  Additionally, if expr is of type RCONST, RLABEL, RDATA,
 *     RAUTO, or RPARAM, it is spilled to the data area and the offset
 *     into the data area is printed.
 */
extern void emitr(const struct expr *);
extern void emitl(const struct expr *);

/*
 * Call frame management.
 *
 * emitpush(expr)
 *     Allocate a scratch register and fill it with the current content
 *     of AC.  The content of AC is undefined afterwards.  Set expr to
 *     refer to it.
 *
 * emitpop(expr)
 *     Deallocate scratch register expr.  Only the most recently
 *     allocated scratch register can be deallocated.  expr is
 *     overwritten with EXPIRED to prevent accidental reuse.  If expr
 *     is not on the stack, this does nothing.
 *
 * newframe(expr)
 *     Start a new call frame and emit a function prologue for a
 *     function named expr.name.  This also generates an appropriate
 *     label.
 *
 * newparam(expr)
 *     Remember expr as a new parameter to the current function and
 *     update expr->value appropriately.  Parameters must be added from
 *     left to right.
 *
 * newauto(expr)
 *     Remembers expr as a new automatic variable and update expr->value
 *     appropriately.
 *
 * ret()
 *     Generate code to return from the current function.  The return
 *     value is whatever is currently in AC.
 *
 * endframe(expr)
 *     End the current call frame and emit the required data.  expr must
 *     be the label corresponding to the beginning of the current
 *     function.  The data looks like this:
 *
 *         number of registers to save, negated
 *         saved registers area
 *         number of arguments, negated
 *         argument storage area
 *         number of template registers to load, negated
 *         frame template
 *         automatic variable area
 */
extern void emitpush(struct expr *);
extern void emitpop(struct expr *);
extern void newframe(const struct expr *);
extern void newparam(struct expr *);
extern void newauto(struct expr *);
extern void ret(void);
extern void endframe(const struct expr *);
