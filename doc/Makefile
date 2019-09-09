# (c) 2019 Robert Clausecker <fuz@fuz.su>

.POSIX:

.SUFFIXES: .ms .pdf .txt .gz

GROFF=groff
NROFF=nroff

all: 8bc.pdf 8bc.txt

.ms.pdf:
	$(GROFF) -Tpdf -t -ms $< >$@

.ms.txt:
	$(NROFF) -c -Tascii -t -ms $< | col >$@

clean:
	rm -f 8bc.pdf 8bc.txt
