# (c) 2019 Robert Clausecker <fuz@fuz.su>

.POSIX:

# to be overwritten by top makefile
bc=8bc
bc1=8bc1
bc1loc=`dirname $$0`/8bc1
palloc=pal
brtloc=`dirname $$0`/brt.pal

CC=c99 -D_POSIX_C_SOURCE=200809L
CFLAGS=-O2

YFLAGS=-d

OBJ=asm.o codegen.o data.o error.o isel.o lexer.o main.o name.o parser.o pdp8.o

all: $(bc) $(bc1)

$(bc1): $(OBJ)
	$(CC) $(LDFLAGS) -o $(bc1) $(OBJ) -ly -ll

$(bc): 8bc.sh
	sed -e 's,%bc1loc%,$(bc1loc),' -e 's,%palloc%,$(palloc),' \
	    -e 's,%brtloc%,$(brtloc),' <8bc.sh >"$(bc)"
	chmod a+x "$(bc)"

lexer.c: lexer.l parser.c

clean:
	rm -f $(bc) $(bc1) parser.c lexer.c y.tab.h $(OBJ)

.PHONY: clean
