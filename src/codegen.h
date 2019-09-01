/*
 * Compute the effect of instruction op with operand e and remember it.
 * Update acstate to the new content of AC.  Possibly emit code.  The
 * instructions JMS, IOT, and OSR, and HLT may not be given.
 *
 * undefer()
 *     emit all deferred instructions such that the
 *     the current machine state corresponds to the simulated state.
 *     This pseudo-instruction is emitted whenever isel is circumvented
 *     to emit code or data into the assembly file directly.
 *
 * iselreset()
 *     forget all deferred instructions and reset the state
 *     machine to a clear accumulator.  This is used at the beginning of
 *     functions and after labels that are only reachable by jumping to
 *     them.
 *
 * acrandom()
 *     forget any knowledge about what AC contains
 *     and assume that its contents are unpredictable.  This is used
 *     whenever isel is circumvented such that AC may have been
 *     modified.
 *
 * lany()
 *     tell isel() that we don't care about what value the L register
 *     has, permitting isel() to set it to an arbitrary value.
 */
extern void isel(int, const struct expr *);
extern void undefer(void);
extern void iselrst(void);
extern void iselacrnd(void);
extern void lany(void);

/*
 * Emit the PDP-8 instruction isn.  Unless isn is an OPR instruction,
 * e is used as an argument to the instruction with an appropriate
 * addressing mode.
 */
extern void emitisn(int, const struct expr *);

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
