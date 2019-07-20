.POSIX:

CC=cc -std=c99 -D_POSIX_C_SOURCE=200809L
CFLAGS=-Os -g -Wall -Wno-parentheses -Wno-missing-braces
YFLAGS=-d

OBJ=asm.o data.o error.o lexer.o main.o name.o parser.o pdp8.o

all: 8bc 8bc1

8bc1: $(OBJ)
	$(CC) -o 8bc1 $(OBJ) -ly -ll

lexer.c: lexer.l parser.c

clean:
	rm -f 8bc 8bc1 parser.c lexer.c y.tab.h $(OBJ)

.PHONY: clean
