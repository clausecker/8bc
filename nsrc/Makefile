.POSIX:

CC=cc -std=c99 -D_POSIX_C_SOURCE=200809L
CFLAGS=-Os -g -Wall -Wno-parentheses -Wno-missing-braces
YFLAGS=-d

OBJ=asm.o error.o lexer.o main.o name.o parser.o pdp8.o

8bc: $(OBJ)
	$(CC) -o 8bc $(OBJ) -ly -ll

lexer.c: lexer.l parser.c

clean:
	rm -f 8bc parser.c lexer.c y.tab.h $(OBJ)

.PHONY: clean
