.RP
.TL
8bc \- a B compiler for the PDP-8
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
Our implementation
.NH 1
The design of 8bc

