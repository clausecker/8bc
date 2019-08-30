.RP
.TL
8bc \- a B Compiler for the PDP-8
.AU
Robert Clausecker \(lafuz@fuz.su\(ra
.AI
Zuse Institute Berlin
.AB
.LP
.I 8bc
is a mostly feature-complete B-compiler for the PDP-8
minicomputer.  In contrast to contemporary B implementations,
.I 8bc
generates native PDP-8 assembly in a single pass.  A deferred
instruction generator omits unnecessary instructions and feeds
information about the current program state back to the code
generator, allowing it to make a situation-dependent choice about
the code generated for high-level language constructs.
.PP
Using this compiler as an example, we explore historical and
contemporary approaches to compiler design and discuss the
mutual influence of and on available computing hardware with a special
focus on the PDP-8 that guided the evolution of modern procedural
languages.
.AE
.NH 1
Introduction
.LP
The B programming language was designed in 1969 by Ken Thompson and
Dennis Ritchie as a simplified version of BCPL for use with the PDP-7
minicomputer [?].  Being the predecessor of C, the B programming
.\" Ritchie, D., "The Development of the C Language", ACM SIGPLAN Notices vol 28 no 3, pp 201--208.
language remains an important milestone in the continued evolution of
procedural languages from first works by Rutishauser [?] over ALGOL 60
.\" something about superplan
and BCPL to B and C and finally to modern incarnations like Limbo, Go,
and Rust.
.PP
The goal of this work is to give an overview over this evolution and to
outline the constraints and features of historical machines as well as
the programming techniques known in the day and how they influenced the
design of contemporary procedural languages and their implementations.
We contrast this with
.I 8bc ,
a rather straightforward implementation of the B language for the PDP-8
minicomputer using modern industry standard tools and a design approach
that started to become commonplace in the 1980s.
.PP
The design of
.I 8bc
and the PDP-8 target architecture was chosen to highlight the
characteristics of the PDP-8, what design concessions have to be made
to bring B on it, and how computers evolved to satisfy the demands of
procedural languages.
.NH 1
Background
.NH 2
The PDP-8 architecture
.NH 2
The B language
.NH 1
Runtime environment and ABI
.NH 2
Historical implementations
.NH 2
The 8bc runtime and ABI
.LP
The
.I 8bc
runtime makes some concessions to deal with the PDP-8's restricted
addressing mode, lack of stack and archaic behaviour of the JSR
instruction.  Instead of generating a stack frame, each B function
has a dedicated \fIcall frame\fR
that stores a template for the zero page, space for the function's
arguments, the function's automatic variables, and the previous
content of the zero page to be restored on return.
.NH 3
Zero page usage
.LP
The zero page is special because it is the only page that can be
addressed directly.  B programs use the zero page as follows:
.DS I
0000 \- 0007\h'2m'interrupt handler
0010 \- 0017\h'2m'indexed registers
0020 \- 0027\h'2m'runtime registers
0030 \- 0177\h'2m'scratch registers
.DE
As interrupts are unsupported by B, the interrupt handler is a single
HLT instruction at address 0001.  Index register 0010 is used to store
one of the factors when the MUL routine is called.  While the B compiler
does not otherwise use the index registers, they are used by the B
runtime routines.
.PP
Scratch registers much be preserved by the callee, indexed registers
need not.  The runtime registers are used to store pointers to
important B runtime functions and as scratch space for those runtime
registers.  The runtime registers are used as follows:
.DS I
0020\h'2m'pointer to the ENTER routine
0021\h'2m'pointer to the LEAVE routine
0022\h'2m'pointer to the MUL routine
0023\h'2m'pointer to the DIV routine
0024\h'2m'pointer to the MOD routine
0025\h'2m'runtime scratch register
0026\h'2m'runtime scratch register
0027\h'2m'runtime scratch register
.DE
.NH 3
Function call sequence
.LP
A function is called with a JSR instruction followed by pointers to the
function's parameters.  The number of parameters must match the number
of arguments in the function's definition, the function returns to the
first instruction after the arguments.
.PP
The call frame looks as follows.  The numbers of registers to save,
function arguments to copy, and registers to initialise are negated to
simplify the ENTER and LEAVE runtime routines.
.DS I
negated number of registers to save
space to save the registers
negated number of arguments
function arguments
negated number of register templates
register templates
space for automatic variables
.DE
The first instruction of every B function calls ENTER, a runtime
function responsible for setting up the environment such that the
function can do its job.  To return, the B function calls LEAVE, a
runtime function that restores the zero page to its previous state
and then returns from the function that called it.
.PP
The ENTER routine first copies all zero page registers that are going
to be used into the call frame.  Then, the parameters are grabbed from
the call site and copied into the call frame.  The return address is
adjusted to skip over them.  Lastly, the register template is copied to
the zero page.  The LEAVE routine is simpler.  It copies the saved
registers back into the zero page and returns to the caller.
.NH 1
The design of 8bc
.NH 2
Optimisations
.LP
.I 8bc
is an optimising compiler.  Even though the lack of an intermediate
code representation makes many optimisations hard to perform, peephole
optimisations are still possible.  To implement these optimisations, the
compiler uses three layers of abstraction in code generation:
.PP
In the
.I parser
layer, each production rule's action generates an instruction sequence
that pops the operands to the implemented operator from a virtual stack,
computes the result, and pushes that result onto the virtual stack.  For
example, a parser action for the + operator could be:
.DS L
.RS
\fIexpr\fR = \fIexpr\fR \(aq+\(aq \fIexpr\fR {
.RS
\fIlda\/\fR(&\fI$3\/\fR);
.br
\fIpop\/\fR(&\fI$3\/\fR);
.br
\fItad\/\fR(&\fI$1\/\fR);
.br
\fIpop\/\fR(&\fI$1\/\fR);
.br
\fIpush\/\fR(&\fI$$\/\fR);
.RE
}
.RE
.DE
The function
.I lda ()
requests for its argument to be loaded into AC and
.I tad ()
requests for a TAD instruction to be generated, implementing the
behaviour of the + operator.  The function
.I push ()
allocates a new memory cell on the virtual stack and writes AC to it,
leaving its contents undefined.  After loading a datum from the stack,
.I pop ()
is used to mark the top of the virtual stack as unused.  Care must be
taken to only pop the top element off the stack.  This is ensured by
always popping operands from right to left.
.PP
Most parser actions are a bit more complicated than this example and
provide multiple instruction sequences for different situations, e.\^g.\&
special casing constant operands.
.PP
These functions
.I lda (),
.I and (),
.I tad (),
.I isz (),
.I dca (),
.I jms (),
.I jmp (),
and
.I opr ()
are implemented in the \fIstack management\fR module to request the
generation of the equivalent instructions.\**
.FS
IOT instructions are never requested and not implemented.
.FE
The module watches the contents of AC and eliminates stack allocations
that can be satisfied by a constant or existing memory location, turning
the virtual stack into stack registers.  When a newly allocated stack
register is immediately loaded back into AC and popped, the entire
.I push ();
.I lda ();
.I pop ();
sequence is discarded, generating no code at all.  Some strength
reductions are performed as well.
.PP
Except for JMS instructions (which are directly emitted), each 
instruction is then passed into the \fIinstruction selection\fR state
machine.  The state machine simulates the effect of the requested
instructions to the extent possible and defers all instructions whose
effect is known at compile time until their effect can no longer be
simulated.
.PP
The state machine keeps track of the contents of AC and L and
continuously replaces the deferred instructions with the shortest
sequence of instructions needed to achieve the same effect;
sequences that compute constants are replaced by sequences of up to
two OPR and TAD instructions, statically known skips are eliminated,
and skips setting AC to 0 or 1 followed by SZA or SNA are merged into
one.
.PP
Summarised, the following optimisations are performed:
.NH 3
Strategy Selection
.LP
An operation is translated into a sequence of instructions depending
on which operands are constant, on the stack, or already in AC.  For
example, a subtraction normally generates the sequence
.DS L
.RS
\fIexpr\fR = \fIexpr\fR \(aq-\(aq \fIexpr\fR {
.RS
\fIlda\fR(&\fI$3\/\fR);
.br
\fIpop\fR(&\fI$3\/\fR);
.br
\fIopr\fR(CMA | IAC);
.br
\fItad\fR(&\fI$1\fR);
.br
\fIpop\fR(&\fI$1\/\fR);
.br
\fIpush\fR(&\fI$$\/\fR);
.RE
}
.RE
.DE
which adds the minuend to the two's complement of the subtrahend.  If
the subtrahend is known to be a constant and the minuend is known to
already be in AC, the sequence
.DS L
.RS
\fIexpr\fR = \fIexpr\fR \(aq-\(aq \fIexpr\fR {
.RS
\fIlda\/\fR(&\fI$1\/\fR);
.br
\fIpop\/\fR(&\fI$1\/\fR);
.br
$3.\fIvalue\fR = RCONST | -\fIval\/\fR($3.\fIvalue\/\fR) & 07777;
.br
\fItad\/\fR(&\fI$3\/\fR);
.br
\fIpush\/\fR(&\fI$$\/\fR);
.RE
}
.RE
.DE
is emitted instead, adding the two's complement of the subtrahend to the
minuend already in AC, saving the minuend from begin deposited on the
stack and then reloaded.
.NH 3
Stack forwarding
.LP
When the content of AC is known to be a constant value or the result of
loading another value, a call to
.I push ()
does not allocate a new stack register but instead returns whatever is
currently in AC.  This eliminates useless stack registers and paves the
way for constant folding.
.NH 3
Reload elimination
.LP
When the content of AC is pushed to the stack and then immediately
loaded into AC and popped, the entire
.I push ();
.I lda ();
.I pop ();
sequence is discarded, leaving the contents of AC untouched.  This
eliminates all unnecessary stack operations during expression evaluation
that are not already caught by stack forwarding.
.NH 3
Double load elimination
.LP
When AC is known to contain the content of a memory location and a load
from that same location is requested, the duplicate load is discarded.
The same optimisation is performed for constants through the constant
folding optimisation.
.NH 3
Strength Reduction
.LP
Instructions which have no effect or can be replaced with OPR
instructions are discarded or replaced.  For example, a
.I tad ()
call that attempts to add 1 to AC is replaced with an IAC instruction.
.NH 3
Constant folding
.LP
Sequences of instructions resulting in a constant value in AC are
deferred.  The entire sequence is then replaced by one or two
instructions loading the desired value into AC.  If possible, OPR
instructions are used to reduce the size of the register template.
.NH 3
Skip elimination
.LP
Skip instructions that can be predicted at compile time are discarded.
If the instruction is known to skip, the skipped instruction is
discarded as well.
.NH 3
Skip forwarding
.LP
A skip instruction that clears AC and is followed by IAC is
recognised as setting AC to the result of the condition.  If such a
sequence is followed by a SZA or SNA microcoded with CLA, the two
skip instructions are merged into one and the IAC is discarded.
.NH 2
Restrictions
.LP
Recursion is not supported.  Due to time constraints, the
.B switch
statement was left out of the implementation.  Implementations for
the / and % operators are missing in
.I brt.pal ,
but can easily be added.  Many common B extensions such as
\fBdo\/\fR-\fBwhile\fR loops, the \fBcontinue\fR statement, or
implementations of & and | with short-circuit behaviour for control
expressions were omitted.
.PP
.I 8bc
directly generates a complete PAL program by concatenating the B runtime
.I brt.pal
and the compiler output.  This runtime contains a rudimentary standard
library comprising the functions
.I exit (),
.I getchar (),
.I putchar (),
and
.I sense ().
No further library functions are provided.  It is not possible to link
two or more B source files into a single binary and there is no way to
write parts of the program in another language.
