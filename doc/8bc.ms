.\" (c) 2019 Robert Clausecker <fuz@fuz.su>
.\" read this in a terminal: nroff -t -ms 8bc.ms | less -R -z66
.\" generate a PDF: groff -Tpdf -t -ms 8bc.ms >8bc.pdf
.RP
.TL
8bc \(en a B Compiler for the PDP-8
.AU
Robert Clausecker \(la\,fuz@fuz.su\/\(ra
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
The B programming language\~[2] was designed in 1969 by Ken Thompson and
Dennis Ritchie as a simplified version of BCPL\~[3] for use with the PDP-7
minicomputer\~[1].  Being the predecessor of C, the B programming
language remains an important milestone in the continued evolution of
procedural languages from first works by Rutishauser over ALGOL 60
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
characteristics of the PDP-8, what design concessions had to be made
to bring B to it, and how computers evolved to satisfy the demands of
procedural languages.
.NH 1
Background
.NH 2
The B language
.LP
The B programming language was designed by Ken Thompson and Dennis
Ritchie of Bell Labs as a simplified variant of the BCPL language for
use on the PDP-7.  The simplifications encompassed a removal of all
language features programmers could make do without as well as a change
from BCPL's wordy ALGOL-like syntax to a syntax built around an
extensive use of punctuation, allowing the programmer to write terser
programs and the parser to be less complex.
.PP
With these simplifications, a self-hosting B compiler was written for
the PDP-7.  B code is compiled to a form of threaded code operating
mainly on a stack of data, conveniently using the same instruction
encoding as the PDP-7\~[4].  The stack comprises of lvalue/rvalue pairs
where the lvalue contains the address of a datum.  If the object on the
stack is not a true lvalue (e.\^g. because it is an intermediate
value), the lvalue field points to the rvalue field containing the
objects rvalue.
.PP
While slightly wasteful, the
.I take-address
operator
.CW &
and the
.I dereference
operator
.CW *
can be implemented easily in this scheme and lvalues do not need to be
distinguished from rvalues for most parts.  In the B runtime for the
PDP-11, this scheme was altered to push rvalue slots to the stack
manually whenever needed.  While making the compiler more complicated,
both memory usage and program performance is improved.
.I 8bc
uses a similar concept born out of the PDP-8's lack of useful addressing
modes.
.NH 3
Syntax and features
.LP
Being the predecessor of C, the feature set of B is best described by
explaining what features where added when B evolved into C.  A more
detailed description can be found in\~[2].
.IP \(bu 2
B is a typeless language; all objects are machine words
.IP \(bu 2
structures, unions, enumerations, type definitions, and type qualifiers
are absent
.IP \(bu 2
consequently, the declaration syntax is simple and does not mention any
of these
.IP \(bu 2
all local variables are automatic; the
.B static
keyword does not exist
.IP \(bu 2
B does not have scopes
.IP \(bu 2 all names external to a function must be
explicitly declared as
.B extrn
(sic!) before use; as a popular extension, some compilers did so
implicitly when an undeclared name was operand to the
.CW ()
operator
.IP \(bu 2
though popular extensions, the
.B continue
statement and \fBdo\fR-\fBwhile\fR loops are absent
.IP \(bu 2
the syntax for combined assignment operators is
.CW = op,
not
.CW = "" op
.IP \(bu 2
the short-circuit logic operators
.CW &&
and
.CW ||
are absent; as a popular extension, many B compilers implemented
.CW &
and
.CW |
with their semantics when used in a control expression
.IP \(bu 2
the operators
.CW ~
and
.CW ^
where absent in early versions of B; the token
.CW ,
is a separator, not an operator
.IP \(bu 2
as with K&R C, the content of the standard library depends on the system
with wildly varying contents
.NH 2
The PDP-8
.LP
The PDP-8 is a family of 12 bit minicomputers introduced by the Digital
Equipment Corporation (DEC) in 1965.  Being among the cheapest computers
on the market while having a large variety of available peripherals an
expansion modules, the PDP-8 and enjoyed a wide usage with over 300,000
units sold over the lifespan of the architecture.  DEC being originally
a supplier of laboratory equipment, intended the PDP-8 to be used as a
low-cost computer for controlling and processing data generated by such
laboratory equipment.  However, its low-price, reliability, and
versatility made computers from the PDP-8 series very popular in other
applications such as health care, industrial process control, time
sharing, and text processing.
.PP
The PDP-8's design is an incremental upgrade over the PDP-5 architecture
from 1963 with only minor changes, like an improved interrupt handling
process and more ways to microcode \f(CROPR\fR instructions.
Peripherals for the original PDP-8 \(lqstraight eight\(rq were
originally taken from the largely compatible pool of PDP-5 peripherals,
but were quickly replaced by newly designed PDP-8 peripherals.  Starting
with the PDP-8/E (for which
.I 8bc
was designed), many machines of the PDP-8 family were built around the
OMNIBUS system bus, enabling the construction of sophisticated expansion
cards that would work on any OMNIBUS system.  Many such cards were
designed for a wide range of applications, greatly contributing to the
popularity of the PDP-8.
.NH 3
Memory reference instructions
.LP
The PDP-8 has an address space of 4'096 words of 12 bits each.  Later
models extend the address space to 8
.I fields
of 4'096 words each for a total of up to 32'768 words.  As extended
memory is not used by
.I 8bc ,
we do not furthe describe it in this article.  Most instructions
operate on a 12 bit accumulator AC and a 1 bit L register that is
connected to the accumulator's carry-out are provided for operation.  It
is often useful to think of L and AC as a single 13 bit accumulator
L:AC.  Being a \fIvon Neumann\fR machine, the PDP-8's program is stored
in the same 4'096 words of memory as the data.  Each instruction
comprises a 3 bit operation code followed by a 9 bit operand.
.PP
The instructions are:
.DS I
0	\f(CRAND\fR	and
1	\f(CRTAD\fR	two's complement add
2	\f(CRISZ\fR	index, skip if zero
3	\f(CRDCA\fR	deposit, clear AC
4	\f(CRJMS\fR	jump to subroutine
5	\f(CRJMP\fR	jump
6	\f(CRIOT\fR	IO transfer
7	\f(CROPR\fR	operate
.DE
The first six instructions are called \fImemory reference
instructions\fR.  Their operand is made of an indirection bit, a page
bit, and a 7 bit address.  If the page bit is set, the address is
completed with the high 5 bits of the program counter, otherwise zeros
are used.  If the indirection bit is set, that address is used to look
up the actual address from memory.  As an example, consider the
instruction 3245 located at address 1757.  The operation code is 3,
indicating a
.CW DCA
instruction.  The P bit is set, so the address 0045 is completed with
1600 from the program counter, yielding an operand of 1645.  The
encoded instruction is thus \f(CRDCA 1645\fR, which deposits the
content of AC into the memory at address 1645 and then clears AC.
.NH 3
Microcoded instructions
.LP
The
.I operate
instruction
.CW OPR
interpretes its operand as a bitmask of operations to execute.  Three
groups of operate instructions exist and within each group, an arbitrary
set of instructions can be
.I microcoded
together to execute at once by computing the bitwise or of their
operation codes.  The first group provides ways
to set up the L and AC registers, increments, and rotations.
.DS I
7200	\f(CRCLA\fR	clear AC
7100	\f(CRCLL\fR	clear L
7040	\f(CRCMA\fR	complement AC
7020	\f(CRCML\fR	complement L
7010	\f(CRRAR\fR	rotate L:AC right
7012	\f(CRRTR\fR	rotate L:AC twice right
7004	\f(CRRAL\fR	rotate L:AC left
7006	\f(CRRTL\fR	rotate L:AC twice left
7002	\f(CRBSW\fR	byte swap AC
7001	\f(CRIAC\fR	increment L:AC
.DE
When multiple group 1 instructions are microcoded together, first
.CW CLA
and
.CW CLL
are executed, then
.CW CMA
and
.CW CML ,
then
.CW IAC ,
and finally
the rotate instructions
.CW RAR ,
.CW RTR ,
.CW RAL ,
.CW RTL ,
and
.CW BSW ,
of which at most one can be microcoded into any given operate
instruction.  Operate instructions from group 1 are commonly used to
aid in implementing complex arithmetic operations.  For example, to
compute \fIA\fR \(OR \fIB\fR, one needs to compute \fIA\fR + \fIB\fR
\- (\,\fIA\fR \(AN \fIB\/\fR):
.DS I
\f(CRCLA      / clear AC
TAD A    / compute 0 + A i.\^e. load A
AND B    / compute A \(AN B
CMA IAC  / compute \(no(A \(AN B) + 1, i.\^e. \-(A \(AN B)
TAD A    / compute A - (A \(AN B)
TAD B    / compute A + B - (A \(AN B), i.\^e. A \(OR B
.DE
Another common purpose is the generation of small immediate
constants, removing the need to place a literal into the current or
zero page.
.PP
Most of operate group 2 provides instructions for conditional execution
depending on the content of L:AC.
Instead of performing conditional jumps, the next instruction is skipped
if the condition holds.  The following microcodable instructions are
available:
.DS I
7600 \f(CRCLA\fR	clear AC
7500 \f(CRSMA\fR	skip on minus AC
7440 \f(CRSZA\fR	skip on zero AC
7420 \f(CRSNL\fR	skip on non-zero L
7410 \f(CRSKP\fR	reverse skip condition
7404 \f(CROSR\fR	or switch registers
7402 \f(CRHLT\fR	halt
.DE
Additionally, the mnemonics
.DS I
7510 \f(CRSPA\fR	skip on positive AC
7450 \f(CRSNA\fR	skip on non-zero AC
7430 \f(CRSZL\fR	skip on zero L
.DE
are provided for skip conditions micro-coded with
.CW SKP .
All of these instructions can be microcoded with each other.  First,
.CW SMA ,
.CW SZA ,
and
.CW SNL
are executed and the next instruction skipped if any of the conditions
holds.  If
.CW SKP
is microcoded in, the skip condition is flipped.  Then,
.CW CLA
is executed and finally
.CW OSR
(sensing the state of the front panel's switches) and
.CW HLT .
With these instructions, unsigned comparisons can be implemented easily.
Due to the lack of an overflow flag it is however rather hard to program
signed (two's complement) comparison and support for such has been
omitted from
.I 8bc
in favour of unsigned comparisons.
.PP
A third group of operate instructions exists but remains unused by
.I 8bc .
This group of instructions manipulates the \fIextended arithmetic
element\fR (EAE), an add-on for the PDP-8/E that provides extra
arithmetic instructions.  We do not consider it any further in thid
document.
.NH 3
Accessing peripherals
.LP
The \fIIO transfer\fR instruction
.CW IOT
is used to communicate with peripherals.  Its operand is split into a
6 bit device number and a 3 bit field each device can interpret as it
wants.  These three bits are commonly used to either provide 8 device
specific operation codes or 3 operation codes that can be
.I microcoded
as desired.  Many devices occupy multiple device numbers with each
device number performing a different set of operations.  Some rare
peripherals even use parts of the device number as additional
operation bits.
.PP
As an example, the serial communication module is implemented as two
devices,
.I keyboard ,
and
.I teleprinter .
The keyboard responds to device 03 and provides the following
microcodable operations:
.DS I
6031	\f(CRKSF\fR	keyboard skip if flag
6032	\f(CRKCC\fR	keyboard clear and read character
6034	\f(CRKRS\fR	keyboard read static
6036	\f(CRKRB\fR	keyboard read and begin next read
.DE
When a character is received through the serial line, an internal
flag is set.  This flag can be queried for with a waiting loop around
.CW KSF .  Once reception of a character is indicated, the flag has to
be reset to allow for the next character to be received and the current
character needs to be transferred to AC.  This can be done with a
.CW KCC
and a
.CW KRS
instruction microcoded into one, giving
.CW KRB .
This yields the following idiom to read a character from the keyboard:
.DS I
\f(CRKSF       / skip if character is ready
 JMP .-1  / if not, loop until it is
KRB       / transfer character to AC\fR
.DE
.PP
Likewise, the teleprinter responds to device 04 and provides the
following microcodable operations to send characters:
.DS I
6041	\f(CRTSF\fR	teleprinter skip if flag
6042	\f(CRTCF\fR	teleprinter clear flag
6044	\f(CRTPC\fR	teleprinter print character
6046	\f(CRTLS\fR	teleprinter load and start
.DE
The operation is similar to the keyboard.  An internal flag is set once
a character has been transmitted and must be manually cleared so the
transmission of the next character can be detected.  This can be done by
microcoding
.CW TCF
with
.CW TPC ,
a combination for which the mnemonic
.CW TLS
is provided.  This yields a common idiom to send a character:
.DS I
\f(CRTSF       / skip if previous character transmitted
 JMP .-1  / if not, loop until it has been
TLS       / send character in AC\fR
.DE
.NH 1
The design of 8bc
.LP
The design of
.I 8bc
was driven by the desire to generate native code for the PDP-8 with
acceptable performance while limiting the size, resource consumption,
and programming techniques of the compiler to the state of the art in
the early 1980's.  This way, we can not only give a good picture of
how to cope with the quirks and constraints of the PDP-8 but also
explore compiler design from a historical perspective.
.NH 2
Runtime environment and ABI
.LP
The
.I 8bc
runtime makes some concessions to deal with the PDP-8's restricted
addressing mode, lack of stack and archaic behaviour of the
.CW JSR
instruction.  Instead of generating a stack frame, each B function
has a dedicated \fIcall frame\fR
that stores a template for the zero page, space for the function's
parameters, the function's automatic variables, and the previous
content of the zero page to be restored on return.
.NH 3
Zero page usage
.LP
The zero page is special because it is the only page that can be
addressed directly.  B programs use the zero page as follows:
.DS I
0000\(en0007	interrupt handler
0010\(en0017	indexed registers
0020\(en0027	runtime registers
0030\(en0177	scratch registers
.DE
As interrupts are unsupported by B, the interrupt handler is a single
.CW HLT
instruction at address 0001.  Index register 0010 is used to store
one of the factors when the
.CW MUL
routine is called.  While the B compiler does not otherwise use the
index registers, they are used by the B runtime routines.
.PP
Scratch registers much be preserved by the callee, indexed registers
need not.  The runtime registers are used to store pointers to
important B runtime functions and as scratch space for those runtime
registers.  The runtime registers are used as follows:
.DS I
0020		pointer to the \f(CRENTER\fR routine
0021		pointer to the \f(CRLEAVE\fR routine
0022		pointer to the \f(CRMUL\fR routine
0023		pointer to the \f(CRDIV\fR routine
0024		pointer to the \f(CRMOD\fR routine
0025		runtime scratch register
0026		runtime scratch register
0027		runtime scratch register
.DE
.NH 3
Function call sequence
.LP
A function is called with a
.CW JSR
instruction followed by pointers to the function's parameters.  The
number of parameters must match the number of parameters in the
function's definition, the function returns to the first instruction
after the arguments.
.PP
The call frame looks as follows.  The numbers of registers to save,
function arguments to copy, and registers to initialise are negated to
simplify the
.CW ENTER
and
.CW LEAVE
runtime routines.
.DS I
negated number of registers to save
space to save the registers
negated number of parameters
function parameters
negated number of register templates
register templates
space for automatic variables
.DE
The first instruction of every B function calls
.CW ENTER ,
a runtime
function responsible for setting up the environment such that the
function can do its job.  To return, the B function calls
.CW LEAVE ,
a runtime function that restores the zero page to its previous state
and then returns from the function that called it.
.PP
The
.CW ENTER
routine first copies all zero page registers that are going
to be used into the call frame.  Then, the arguments are grabbed from
the call site and copied into the call frame.  The return address is
adjusted to skip over them.  Lastly, the register template is copied to
the zero page.  The
.CW LEAVE
routine is simpler: it copies the saved
registers back into the zero page and returns to the caller.
.NH 2
Program structure
.LP
Like modern and historical C compilers,
.I 8bc
is split into a compiler driver
.I 8bc
that passes the source file through compiler and assembler, interpretes
options, prepends the B runtime
.I brt.pal ,
and finally deletes intermediate files, and an actual compiler
.I 8bc1
that translates B source into PAL assembly.  This compiler is a one pass
compiler written in C using
.B lex (1)
and
.B yacc (1)
to generate lexer and parser.  Contrary to historical B and C compilers
(but not compilers for other languages such as Pascal), no intermediate
representation of the source code is used.  Instead, code is generated
at each parser action.  This makes for a very memory and time efficient
design, but greatly restricts the amounts of optimisations possible.
.PP
Apart from a few global variables, the majority of the state remembered
by the compiler is found in a \fIdefinition table\fR for variables and
functions defined at the top level and a \fIdeclaration table\fR for
names declared within a function.  While the declaration table is vital
for the compiler to find out about the storage class about local names,
the definition table is only needed due to shortcomings of the PAL
assembler: as it is limited to symbols of up to 7 alphanumeric
characters, we cannot always use B names as symbol names.  Instead, the
B compiler translates all names to numbered labels with the association
between name and number being kept in the declaration and definition
tables.
.NH 2
Character set
.LP
.I 8bc
compiles B source files written in ASCII.  To allow for source files to
be composed on a real PDP-8 using an ASR 33 teletype, a 6 bit ASCII
representation is used internally, mapping ASCII codes 0140\(en0176 to
0100\(en0140, yielding the character set:
.DS I
.TS
cw20 cw20 .
\fBnormal	alternative\fR
_
\f(CR\0 ! " # $ % & \(aq
( ) * + , - . /
0 1 2 3 4 5 6 7
8 9 : ; < = > ?
@ A B C D E F G	\` a b c d e f g
H I J K L M N O	h i j k l m n o
P Q R S T U V W	p q r s t u v w
X Y Z [ \\ ] \(ha _	x y z { | } \(ti \0
_
.TE
.DE
The alternative characters are treated equally to the normal characters
except inside character or string literals.  Names and keywords are case
insensitive.  There is no alternative character for the
.CW _
character.  For example, the program
.DS I
\f(CRmain() {
	extrn putchar;
	auto i 0, hello "Hello, World!*n";

	while (hello[i] != \(aq*e\(aq)
		putchar(hello[i++]);
}
.DE
could equally be written as
.DS I
\f(CRMAIN() [
	EXTRN PUTCHAR;
	AUTO I 0, HELLO "Hello, World!*N";

	WHILE (HELLO[I] != \(aq*E\(aq)
		PUTCHAR(HELLO[I++]);
]
.DE
While case is honoured inside string and character literals, the ASR 33
teletype is unable to read or print ASCII characters from the
.I alternative
characters list and prints the corrsponding
.I normal
characters instead.  To provide UNIX-like semantics, the
.I getchar ()
function translates CR to LF and clears the parity bit; the function
.I putchar ()
inserts a CR before each LF.
.NH 2
Storage classes
.LP
.I 8bc
recognises 7 \fIstorage classes\fR.  The storage class is used
by the code generator to figure out how to refer to an object.  Each
storage class exists as an
.I lvalue
and as an
.I rvalue
class.  The difference is that the rvalue storage class has an
additional level of indirection.  For example, an object of storage class
.CW RLABEL
is the value of a label.  If we dereference an
.CW RLABEL ,
we get an object of storage class
.CW LLABEL
which is the object located at that label.  The lvalue storage classes are:
.DS I
0	\f(CRLCONST\fR	object at absolute address
1	\f(CRLVALUE\fR	object pointed to by zero page register
2	\f(CRLLABEL\fR	object pointed to by label
3	\f(CRLDATA\fR	object in data area
4	\f(CRLSTACK\fR	object pointed to by stack register
5	\f(CRLAUTO\fR	object in automatic variable area
6	\f(CRLPARAM\fR	object in parameter area
.DE
The storage class
.CW RCONST
is used for constants.  \fIStack register\fR
refers to a register in the zero page used to spill temporary values.
The first stack register follows the last register loaded from the zero
page template.  Since the size of the zero page template is only known
after the function has been compiled, the compiler refers to stack
registers through an offset from a label referring to the first stack
register, necessitating a separate storage class.
.PP
A B object is converted from lvalue to rvalue through the & (take
address) operator and back through the * (dereference) operator.  When
an object that is not of class
.CW LVALUE ,
.CW LSTACK ,
.CW RVALUE ,
or
.CW RSTACK
is used as an operand to a memory instruction (one of
.CW AND ,
.CW TAD ,
.CW ISZ ,
.CW DCA ,
.CW JMS ,
or
.CW JMP ),
the object is
.I spilled
by templating a zero page register with the object's rvalue and
substituting an object of type
.CW LVALUE
or
.CW RVALUE
referring to said zero page register to render the original object
accessible.  Objects of the various storage classes are otherwise
created as follows:
.DS I
\f(CRRCONST\fR	numerical or character constant
\f(CRLLABEL\fR	external variable, function, or label
\f(CRLDATA\fR	string constant
\f(CRRSTACK\fR	value of an expression
\f(CRLAUTO\fR	automatic variable
\f(CRLPARAM\fR	function parameter
.DE
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
.DS I
\fIexpr\fR = \fIexpr\fR \(aq+\(aq \fIexpr\fR {
	\fIlda\/\fR(&\fI$3\/\fR);
	\fIpop\/\fR(&\fI$3\/\fR);
	\fItad\/\fR(&\fI$1\/\fR);
	\fIpop\/\fR(&\fI$1\/\fR);
	\fIpush\/\fR(&\fI$$\/\fR);
}
.DE
The function
.I lda ()
requests for its argument to be loaded into AC and
.I tad ()
requests for a
.CW TAD
instruction to be generated, implementing the behaviour of the + operator.
The function
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
.CW IOT
instructions are never requested and not implemented.
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
Except for
.CW JMS
instructions (which are directly emitted), each 
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
two
.CW OPR
and
.CW TAD
instructions, statically known skips are eliminated, and skips setting
AC to 0 or 1 followed by
.CW SZA
or
.Cw SNA
are merged into one.
.PP
Summarised, the following optimisations are performed:
.NH 3
Strategy Selection
.LP
An operation is translated into a sequence of instructions depending
on which operands are constant, on the stack, or already in AC.  For
example, a subtraction normally generates the sequence
.DS I
\fIexpr\fR = \fIexpr\fR \(aq-\(aq \fIexpr\fR {
	\fIlda\/\fR(&\fI$3\/\fR);
	\fIpop\/\fR(&\fI$3\/\fR);
	\fIopr\/\fR(\f(CRCMA\fR | \f(CRIAC\fR);
	\fItad\/\fR(&\fI$1\fR);
	\fIpop\/\fR(&\fI$1\/\fR);
	\fIpush\/\fR(&\fI$$\/\fR);
}
.DE
which adds the minuend to the two's complement of the subtrahend.  If
the subtrahend is known to be a constant and the minuend is known to
already be in AC, the sequence
.DS I
\fIexpr\fR = \fIexpr\fR \(aq-\(aq \fIexpr\fR {
	\fIlda\/\fR(&\fI$1\/\fR);
	\fIpop\/\fR(&\fI$1\/\fR);
	\fI$3.value\fR = \f(CRRCONST\fR | \-\fIval\/\fR(\fI$3.value\/\fR) & 07777;
	\fItad\/\fR(&\fI$3\/\fR);
	\fIpush\/\fR(&\fI$$\/\fR);
}
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
Instructions which have no effect or can be replaced with
.CW OPR
instructions are discarded or replaced.  For example, a
.I tad ()
call that attempts to add 1 to AC is replaced with an
.CW IAC
instruction.
.NH 3
Constant folding
.LP
Sequences of instructions resulting in a constant value in AC are
deferred.  The entire sequence is then replaced by one or two
instructions loading the desired value into AC.  If possible,
.CW OPR
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
A skip instruction that clears AC and is followed by
.CW IAC
is recognised as setting AC to the result of the condition.  If such a
sequence is followed by a
.CW SZA
or
.CW SNA
microcoded with
.CW CLA ,
the two skip instructions are merged into one and the
.CW IAC
is discarded.
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
expressions were omitted.  Redefinitions and use of undefined functions
or variables are not detected by the compiler but will lead to failure
during assembly.
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
.NH 1
Literature
.IP 1
RITCHIE, Dennis M. \fIThe development of the C language.\fR ACM SIGPLAN Notices, 1993, vol\~28, no\~3, p\~201\(en208.
.IP 2
THOMPSON, Ken. \fIUsers' Reference to B.\fR Bell Laboratories, 1972, MM-72-1271-1.
.IP 3
RICHARDS, Martin. \fIThe BCPL Reference Manual.\fR Multics Repository, 1967, Memorandum M-352.
.IP 4
PAPENHOFF, Angelo. \fIThe B Programming Language.\fR http://squoze.net/B, 2018.
