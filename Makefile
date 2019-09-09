# (c) 2019 Robert Clausecker

.POSIX:

# name of this package
package=8bc
#package=pdp8bc

# adjust these according to your systems demands
CC=c99 -D_POSIX_C_SOURCE=200809L
#CC=gcc -std=c99
CFLAGS=-O2
#CFLAGS=-Os -g -Wall -Wno-parentheses -Wno-missing-braces
LDFLAGS=-s
YACC=yacc
#YACC=bison -y
LEX=lex
#LEX=flex -X
GROFF=groff
NROFF=nroff

# installation directories
# usually, only PREFIX needs adjustment
PREFIX=/usr/local
BINDIR=$(PREFIX)/bin
LIBEXECDIR=$(PREFIX)/libexec
DATADIR=$(PREFIX)/share
DOCDIR=$(DATADIR)/doc
EXAMPLEDIR=$(DATADIR)/example
MANDIR=$(DATADIR)/man
MAN1DIR=$(MANDIR)/man1

# prefix for installation paths during install target
# useful when you want to build packages
PROTO=
#PROTO=proto

# name of the 8bc binary
# if you are a maintainer and 8bc/8bc1/pal violate any of your
# guidelines, please use pdp8bc/pdp8bc1/pdp8pal instead.
bc=8bc
bcupper=8BC
bc1=8bc1
pal=pal
palupper=PAL
#bc=pdp8bc
#bcupper=PDP8BC
#bc1=pdp8bc1
#pal=pdp8pal
#palupper=PDP8PAL

# what makes up this distribution
# apart from the files named above
doc=8bc.pdf 8bc.txt
example=dump.b echo.b fibonacci.b gcd.b hello.b multiply.b numbers.b \
    sort.b words.b

bc1loc=$(LIBEXECDIR)/$(bc1)
palloc=$(BINDIR)/$(pal)
brtloc=$(DATADIR)/$(package)/brt.pal

# what we pass down to other make processes
makeopt="CC=$(CC)" "CFLAGS=$(CFLAGS)" "LDFLAGS=$(LDFLAGS)" \
    "YACC=$(YACC)" "LEX=$(LEX)" "GROFF=$(GROFF)" "NROFF=$(NROFF)" \
	bc=$(bc) bc1=$(bc1) "bc1loc=$(bc1loc)" "palloc=$(palloc)" \
	"brtloc=$(brtloc)"

all: $(pal) $(bc).1 $(pal).1
	cd doc && $(MAKE) $(makeopt) all
	cd src && $(MAKE) $(makeopt) all

# patch the pal assembler
$(pal).c: contrib/pal.c contrib/pal.diff
	cp contrib/pal.c "$(pal).c"
	patch "$(pal).c" <contrib/pal.diff
	rm -f "$(pal).c.orig"

# patch the documentation
$(bc).1: doc/8bc.1
	sed -e 's,%bc%,$(bc),' -e 's,%pal%,$(pal),' \
	    -e 's,%brtloc%,$(brtloc),' -e 's,%bcupper%,$(bcupper),' \
	    -e 's,%bc1loc%,$(bc1loc),' <doc/8bc.1 >$(bc).1

$(pal).1: doc/pal.1
	sed -e 's,%bc%,$(bc),' -e 's,%pal%,$(pal),' \
	    -e 's,%palupper%,$(palupper),' <doc/pal.1 >$(pal).1

# copy files to installation directory
install: all
	mkdir -p "$(PROTO)/$(BINDIR)"
	cp "src/$(bc)" "$(pal)" "$(PROTO)/$(BINDIR)/"
	mkdir -p "$(PROTO)/$(LIBEXECDIR)"
	cp "src/$(bc1)" "$(PROTO)/$(bc1loc)"
	mkdir -p "$(PROTO)/$(DATADIR)/$(package)"
	cp "src/brt.pal" "$(PROTO)/$(brtloc)"
	mkdir -p "$(PROTO)/$(DOCDIR)/$(package)"
	for doc in $(doc) ; do cp doc/$$doc "$(PROTO)/$(DOCDIR)/$(package)/" ; done
	mkdir -p "$(PROTO)/$(EXAMPLEDIR)/$(package)"
	for example in $(example) ; do cp example/$$example "$(PROTO)/$(EXAMPLEDIR)/$(package)/" ; done
	mkdir -p "$(PROTO)/$(MAN1DIR)"
	cp "$(bc).1" "$(pal).1" "$(PROTO)/$(MAN1DIR)/"

clean:
	cd doc && $(MAKE) $(makeopt) clean
	cd src && $(MAKE) $(makeopt) clean
	rm -f $(pal) $(pal).c $(pal).1 $(bc).1

.PHONY: all install clean
