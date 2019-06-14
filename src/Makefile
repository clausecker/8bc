.POSIX:

CC=c99 -D_POSIX_C_SOURCE=200809L


YFLAGS=-d

8bc: parser.o lex.o
	$(CC) -o 8bc parser.o lex.o -ly

lex.c: lex.l parser.c

