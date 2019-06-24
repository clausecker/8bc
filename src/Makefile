.POSIX:

CC=cc -std=c99 -D_POSIX_C_SOURCE=200809L
CFLAGS=-O3 -g -Wall -Wno-parentheses
YFLAGS=-d

OBJ=parser.o lex.o name.o

8bc: $(OBJ)
	$(CC) -o 8bc $(OBJ) -ly -ll

lex.c: lex.l parser.c

clean:
	rm -f 8bc parser.c lex.c y.tab.h *.o

.PHONY: clean
