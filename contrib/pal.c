/* file:    pal.c

   purpose: a 2 pass PDP-8 pal-like assembler

   author:  Douglas Jones <jones@cs.uiowa.edu> -- built basic bits
            Rich Coon <coon@convexw.convex.com> -- added enough handle OS/278
            Bernhard Baehr <b.baehr@madsack.de> -- patch to correct checksum

   disclaimer:  This assembler accepts a subset of the PAL 8 assembly language.
	It was written as a quick hack in order to download software into my
	bare-bones PDP-8 system, and no effort has been made to apply sound
	software engineering techniques in its design.  See the symbol table
	and code for the set of pseudo-ops supported.  See DEC's "Programming
	Languages (for the PDP/8)" for complete documentation of pseudo-ops,
	or see DEC's "Introduction to Programming (for the PDP/8)" or a
	lower level introduction to the assembly language.

   use: After assembly on a UNIX system, this program should be stored as
	the pal command.  This program uses the following file name extensions

		.pal	source code (input)
		.lst	assembly listing (output)
		.bin	assembly output in DEC's bin format (output)
		.rim	assembly output in DEC's rim format (output)

	This program takes the following command line switches

		-d	dump the symbol table at end of assembly
		-r	produce output in rim format (default is bin format)

   known bugs:  Only a minimal effort has been made to keep the listing
	format anything like the PAL-8 listing format.  It screams too loud
	for off-page addresses (a quote mark would suffice), and it doesn't
	detect off-page addresses it can't fix (it should scream about them).

	Under DOS (and other systems that distinguish between text and binary
	files), the object output file must be opened in binary mode, so the
	call fopen(objname, "w") must be rewritten as fopen(objname, "wb").
	(This change must be made on or around line 1175 in this file.)

   warrantee:  If you don't like it the way it works or if it doesn't work,
	that's tough.  You're welcome to fix it yourself.  That's what you
	get for using free software.

   copyright notice:  I wrote this and gave it away for free; if you want to
	make changes and give away the result for free, that's fine with me.
	If you can fix it enough that you can sell it, ok, but don't put any
	limits on the purchaser's right to do the same.  All the above aside,
	if you improve it or fix any bugs, it would be nice if you told me
	and offered me a copy of the new version.
*/

#include <stdio.h>
#include <ctype.h>
#define isend(c) ((c=='\0')||(c=='\n'))
#define isdone(c) ( (c == '/') || (isend(c)) || (c == ';') )

/* connections to command line */
#define NAMELEN 128
FILE *in;		/* input file */
char *filename = NULL;	/* input file's name */
FILE *obj = NULL;	/* object file */
FILE *objsave = NULL;	/* alternate object file (supports ENPUNCH/NOPUNCH) */
char objname[NAMELEN];	/* object file's name */
FILE *lst = NULL;	/* listing file */
FILE *lstsave = NULL;	/* alternate listing file */
char lstname[NAMELEN];	/* listing file's name */
int dumpflag = 0;	/* dump symtab if 1 (defaults to no dump) */
int rimflag = 0;	/* generate rim format if 1 (defaults bin) */
int cksum = 0;		/* checksum generated for .bin files */
int errors = 0;		/* number of errors found so far */

/* return c as upper-case if it is lower-case; else no change */
int c2upper(c)
int c;
{
	return ((c >= 'a' && c <= 'z') ? (c - ('a'-'A')) : c);
}

/* command line argument processing */
getargs(argc, argv)
int argc;
char *argv[];
{
	int i,j;

        for (i=1; i < argc; i++) {
		if (argv[i][0] == '-') { /* a flag */
			for (j=1; argv[i][j] != 0; j++) {
				if (argv[i][1] == 'd') {
					dumpflag = 1;
				} else if (argv[i][1] == 'r') {
					rimflag = 1;
				} else {
					fprintf( stderr,
						 "%s: unknown flag: %s\n",
						 argv[0], argv[i] );
					fprintf( stderr,
						 " -d -- dump symtab\n" );
					fprintf( stderr,
						 " -r -- output rim file\n" );
					exit(-1);
				}
			}
		} else { /* no - at front of argument, must be file name */
			if (filename != NULL) {
				fprintf( stderr, "%s: too many input files\n",
					 argv[0] );
				exit(-1);
			}
			filename = &argv[i][0];
		} /* end if */
        } /* end for loop */

        if (filename == NULL) { /* no input file specified */
		fprintf( stderr, "%s: no input file specified\n", argv[0] );
		exit(-1);
	}

	/* now, fix up the file names */
	{
		int dot; /* location of last dot in filename */
		dot = 0;
		for (j = 0; filename[j] != 0; j++) {
			if (j > (NAMELEN - 5)) {
				fprintf( stderr,
					 "%s: file name too long %s\n",
					 argv[0], filename );
				exit(-1);
			}
			lstname[j] = objname[j] = filename[j];
			if (filename[j]=='.') {
				dot = j;
			} else if (filename[j]=='/') {
				dot = 0;
			}
		}
		if (dot == 0) {
			dot = j;
			lstname[dot] = objname[dot] = '.';
		}
		lstname[dot + 1] = 'l';
		lstname[dot + 2] = 's';
		lstname[dot + 3] = 't';
		lstname[dot + 4] = '\0';

		if (rimflag == 1) {
			objname[dot + 1] = 'r';
			objname[dot + 2] = 'i';
			objname[dot + 3] = 'm';
		} else {
			objname[dot + 1] = 'b';
			objname[dot + 2] = 'i';
			objname[dot + 3] = 'n';
		}
		objname[dot + 4] = '\0';
        }

	/* now, open up the input file */
	if ((in = fopen(filename, "r")) == NULL) {
		fprintf( stderr, "%s: cannot open %s\n", argv[0], filename );
		exit(-1);
	}
}

/* symbol table */
#define SYMBOLS  1024
#define SYMLEN 7
struct symbol {
	char sym[SYMLEN]; /* the textual name of the symbol, zero filled */
	short int val;	  /* the value associated with the symbol */
} symtab[SYMBOLS] = {  /* values = 01xxxx indicate MRI */
		       /* values = 04xxxx indicate pseudo-ops */

	"DECIMAL", 040000, /* read literal constants in base 10 */
	"OCTAL"  , 040001, /* read literal constants in base 8 */
	"ZBLOCK" , 040002, /* zero a block of memory */
	"PAGE"   , 040003, /* advance origin to next page or page x (0..37) */
	"TEXT"   , 040004, /* pack 6 bit trimmed ASCII into memory */
	"EJECT"  , 040005, /* eject a page in the listing */
	"FIELD"  , 040006, /* set origin to memory field */
	"NOPUNCH", 040007, /* turn off object code generation */
	"ENPUNCH", 040010, /* turn on object code generation */
	"XLIST"  , 040011, /* toggle listing generation */
	"IFZERO" , 040012, /* unsupported */
	"IFNZRO" , 040013, /* unsupported */
	"IFDEF"  , 040014, /* unsupported */
	"IFNDEF" , 040015, /* unsupported */
	"RELOC"  , 040016, /* assemble for execution at a different address */
	"SEGMNT" , 040017, /* like page, but with page size = 1K words */
	"BANK"   , 040020, /* like field, select a different 32K out of 128K */
	"FIXMRI" , 040021, /* like =, but creates mem ref instruction */

	"AND", 010000, /* mainline instructions */
	"TAD", 011000,
	"ISZ", 012000,
	"DCA", 013000,
	"JMS", 014000,
	"JMP", 015000,
	"I",   010400,
	"Z",   010000,

	"NOP", 007000, /* group 1 */
	"CLA", 007200,
	"CIA", 007041,
	"CLL", 007100,
	"CMA", 007040,
	"CML", 007020,
	"IAC", 007001,
	"BSW", 007002,
	"RAR", 007010,
	"RAL", 007004,
	"RTR", 007012,
	"RTL", 007006,
	"STA", 007240,
	"STL", 007120,
	"GLK", 007204,
	"LAS", 007604,
	

        "SMA", 007500, /* group 2 */
	"SZA", 007440,
	"SNL", 007420,
	"SKP", 007410,
	"SPA", 007510,
	"SNA", 007450,
	"SZL", 007430,
	"OSR", 007404,
	"HLT", 007402,

	"KCC", 006032, /* actually iots, but haven't fixed iots yet */
	"KSF", 006031,
	"KRS", 006034,
	"KRB", 006036,
	"IOT", 006000,
	"ION", 006001,
	"IOF", 006002,
	"CDF", 006201,
	"CIF", 006202,
	"RDF", 006214,
	"RIF", 006224,
	"RIB", 006234,
	"RMF", 006244,
	"TSF", 006041,
	"TCF", 006042,
	"TPC", 006044,
	"TLS", 006046,
	"RSF", 006011,
	"RRB", 006012,
	"RFC", 006014,
	"PSF", 006021,
	"PCF", 006022,
	"PPC", 006024,
	"PLS", 006026
};

/* the following define is based on a careful count of the above entries */
#define firstsym 75

/* dump symbol table */
dump()
{
	int i;
	fprintf( lst, "\n" );
	for (i = firstsym; symtab[i].sym[0] != '\0'; i++) {
		fprintf( lst, "%s	%4.4o\n",
			 symtab[i].sym, symtab[i].val );
	}
}

/* define symbol */
define( sym, val )
char sym[SYMLEN];
short int val;
{
	int i,j;
	for (i = 0; symtab[i].sym[0] != '\0'; i++) {
		for (j = 0; j < SYMLEN; j++) {
			if (symtab[i].sym[j] != sym[j]) {
				goto mismatch;
			}
		}
		goto match;
		mismatch:;
	}
	/* if it ever gets here, a new symbol must be defined */
	if (i < (SYMBOLS - 1)) {
		for (j = 0; j < SYMLEN; j++) {
			symtab[i].sym[j] = sym[j];
		}
	}
	match:;
	symtab[i].val = val;
}

short int lookup( sym )
char sym[SYMLEN];
{
	int i,j;
	for (i = 0; symtab[i].sym[0] != '\0'; i++) {
		for (j = 0; j < SYMLEN; j++) {
			if (symtab[i].sym[j] != sym[j]) {
				goto mismatch;
			}
		}
		goto match;
		mismatch:;
	}
	/* if it ever gets here, the symbol is undefined */
	return -1;

	match:;
	return symtab[i].val;
}

/* single pass of the assembler */

#define LINELEN 96
char line[LINELEN];
int pos;    /* position on line */
int listed; /* has line been listed to listing yet (0 = no, 1 = yes) */
int lineno; /* line number of current line */


listline()
/* generate a line of listing if not already done! */
{
	if ((lst != NULL) && (listed == 0)) {
		fprintf( lst, "%4d            ", lineno );
		fputs( line, lst );
	}
	listed = 1;
}

error( msg )
char *msg;
/* generate a line of listing with embedded error messages */
{
	FILE *tlst;
	if (lst == NULL) { /* force error messages to print despite XLIST */
		tlst = lstsave;
	} else {
		tlst = lst;
	}
	if (tlst != NULL) {
		listline();
		fprintf( tlst, "%-15.15s ", msg );
		{
			int i;
			for (i = 0; i < (pos-1); i++) {
				if (line[i] == '\t') {
					putc('\t', tlst );
				} else {
					putc( ' ', tlst );
				}
			}
		}
		fputs( "^\n", tlst );
		fprintf( stderr, "%4d  %s\n", lineno, msg );
	}
	listed = 1;
	errors++;
}

readline()
/* read one input line, setting things up for lexical analysis */
{
	listline(); /* if it hasn't been listed yet, list it! */
	lineno = lineno + 1;
	listed = 0;
	pos = 0;
	if (fgets( line, LINELEN-1, in ) == NULL) {
		line[0] = '$';
		line[1] = '\n';
		line[2] = '\000';
		error( "end of file" );
	}
}

putleader()
/* generate 2 feet of leader on the object file, as per DEC documentation */
{
	if (obj != NULL) {
		int i;
		for (i = 1; i < 240; i++) {
 			fputc( 0200, obj );
		}
	}
}

puto(c)
int c;
/* put one character to obj file and include it in checksum */
{
	c &= 0377;
	fputc(c, obj);
	cksum += c;
}

int field; /* the current field */

putorg( loc )
short int loc;
{
	puto( ((loc >> 6) & 0077) | 0100 );
	puto( loc & 0077 );
}

putout( loc, val )
short int loc;
short int val;
/* put out a word of object code */
{
	if (lst != NULL) {
		if (listed == 0) {
			fprintf( lst, "%4d %1.1o%4.4o %4.4o ",
				 lineno, field, loc, val);
			fputs( line, lst );
		} else {
			fprintf( lst, "     %1.1o%4.4o %4.4o ",
				 field, loc, val);
			putc( '\n', lst );
		}
	}
	if (obj != NULL) {
		if (rimflag == 1) { /* put out origin in rim mode */
			putorg( loc );
		}
		puto( (val >> 6) & 0077 );
		puto( val & 0077 );
	}
	listed = 1;
}


char lexstart; /* index of start of the current lexeme on line */
char lexterm;  /* index of character after the current lexeme on line */

#define isblank(c) ((c==' ')||(c=='\t')||(c=='\f')||(c=='>'))

nextlex()
/* get the next lexeme into lex */
{
	while (isblank(line[pos])) {
		pos++;
	}

	lexstart = pos;

	if (isalpha(line[pos])) { /* identifier */
		while (isalnum(line[pos])) {
			pos++;
		}
	} else if (isdigit(line[pos])) { /* number */
		while (isdigit(line[pos])) {
			pos++;
		}
	} else if (line[pos] == '"') { /* quoted letter */
		pos ++;
		pos ++;
	} else if (isend(line[pos])) { /* end of line */
		/* don't advance pos! */
	} else if (line[pos] == '/') { /* comment */
		/* don't advance pos! */
	} else { /* all punctuation is handled here! */
		pos++;
	}

	lexterm = pos;
}

deflex( start, term, val )
int start; /* start of lexeme to be defined */
int term; /* character after end of lexeme to be defined */
int val; /* value of lexeme to be defined */
{
	char sym[SYMLEN];
	int from, to;

	from = start;
	to = 0;
	while ((from < term) && (to < SYMLEN)) {
		sym[to++] = c2upper(line[from++]);
	}
	while (to < SYMLEN) {
		sym[to++] = '\000';
	}

	define( sym, val );
}


condtrue()
/* called when a true conditional has been evaluated */
/* lex should be the opening <; skip it and setup for normal assembly */
{
	if (line[lexstart] == '<') {
		nextlex(); /* skip the opening bracket */
	} else {
		error( "< expected" );
	}
}

condfalse()
/* called when a false conditional has been evaluated */
/* lex should be the opening <; ignore all text until the closing > */
{
	if (line[lexstart] == '<') {
		int level = 1;
		/* invariant: line[pos] is the next unexamined character */
		while (level > 0) {
			if (isend(line[pos])) { /* need to get a new line */
				readline();
			} else if (line[pos] == '>') {
				level --;
				pos++;
			} else if (line[pos] == '<') {
				level ++;
				pos++;
			} else if (line[pos] == '$') {
				level = 0;
				pos++;
			} else {
				pos++;
			}
		}
		nextlex();
	} else {
		error( "< expected" );
	}
}

int lc; /* the location counter */
int reloc; /* the relocation distance (see RELOC) */
int pzlc; /* the page zero location counter for page zero constants */
int cplc; /* the current page location counter for current page constants */
int radix; /* the default number radix */

int pz[0200]; /* storehouse for page zero constants */
int cp[0200]; /* storehouse for current page constants */

putpz()
/* put out page zero data */
{
	int loc;
	if (pzlc < 00177) {
		if (rimflag != 1) { /* put out origin if not in rim mode */
			if (obj != NULL) {
				putorg( pzlc+1 );
			}
		}
		for (loc = pzlc+1; loc <= 00177; loc ++) {
			putout( loc, pz[loc] );
		}
	}
	pzlc = 00177;
}

putcp()
/* put out current page data */
{
	int loc;
	if (cplc < 00177) {
		if (lc > (cplc + (lc & 07600))) { /* overrun constant pool */
			error( "overrun" );
		}
		if (rimflag != 1) { /* put out origin if not in rim mode */
			if (obj != NULL) {
				putorg( cplc+1 + (lc & 07600) );
			}
		}
		for (loc = cplc+1; loc <= 00177; loc ++) {
			putout( loc + (lc & 07600), cp[loc] );
		}
	}
	cplc = 00177;
}

int getexprs(); /* forward declaration */

int evalsym()
/* get the value of the current identifier lexeme; don't advance lexeme */
{
	char sym[SYMLEN];
	int from = lexstart;
	int to = 0;

	/* assert isalpha(line[lexstart]) */

	/* copy the symbol */
	while ((from < lexterm) && (to < SYMLEN)) {
		sym[to++] = c2upper(line[from++]);
	}
	while (to < SYMLEN) {
		sym[to++] = '\000';
	}

	return (lookup( sym ));
}

int delimiter; /* the character immediately after this eval'd term */
nextlexblank()
/* used only within eval, getexpr, this prevents illegal blanks */
{
	nextlex();
	if (isblank(delimiter)) {
		error("illegal blank");
	}
	delimiter = line[lexterm];
}

int eval()
/* get the value of the current lexeme, set delimiter and advance */
{
	int val;

	delimiter = line[lexterm];
	if (isalpha(line[lexstart])) {
		val = evalsym();

		if (val == -1) {
			error( "undefined" );
			nextlex();
			return 0;
		} else {
			nextlex();
			return val;
		}

	} else if (isdigit(line[lexstart])) {
		int from = lexstart;

		val = 0;
		while (from < lexterm) {
			int digit = (int)line[from++] - (int)'0';
			if (digit < radix) {
				val = (val * radix) + digit;
			} else {
				error("d > radix");
			}
		}
		nextlex();
		return val;

	} else if (line[lexstart] == '"') {
		val = line[lexstart+1] | 0200;
		delimiter = line[lexstart+2];
		pos = lexstart+2;
		nextlex();
		return val;

	} else if (line[lexstart] == '.') {
		val = (lc + reloc) & 07777;
		nextlex();
		return val;

	} else if (line[lexstart] == '[') {
		int loc;

		nextlexblank(); /* skip bracket */
		val = getexprs() & 07777;
		if (line[lexstart] == ']') {
			nextlexblank(); /* skip end bracket */
		} else {
			/* error("parens") */;
		}

		loc = 00177;
		while ((loc > pzlc) && (pz[loc] != val)) {
			loc--;
		}
		if (loc == pzlc) {
			pz[pzlc] = val;
			pzlc--;
		}
		return loc;

	} else if (line[lexstart] == '(') {
		int loc;

		if ((lc & 07600) == 0) {
			error("page zero");
		}
		nextlexblank(); /* skip paren */
		val = getexprs() & 07777;
		if (line[lexstart] == ')') {
			nextlexblank(); /* skip end paren */
		} else { /*
			error("parens") */ ;
		}

		loc = 00177;
		while ((loc > cplc) && (cp[loc] != val)) {
			loc--;
		}
		if (loc == cplc) {
			cp[cplc] = val;
			cplc--;
		}
		return loc + ((lc + reloc) & 07600);

	}
	error("value");
}

int getexpr()
/* get an expression, from the current lexeme onward, leave the current
   lexeme as the one after the expression!

   Expressions contain terminal symbols (identifiers) separated by operators
*/
{
	int value;

	delimiter = line[lexterm];
	if (line[lexstart] == '-') {
		nextlexblank();
		value = -eval();
	} else {
		value = eval();
	}

more:	/* here, we assume the current lexeme is the operator
           separating the previous operand from the next, if any */

	if (isblank(delimiter)) {
		return value;
	}

	/* assert line[lexstart] == delimiter */

	if (line[lexstart] == '+') { /* add */

		nextlexblank(); /* skip over the operator */
		value = value + eval();
		goto more;

	}
	if (line[lexstart] == '-') { /* subtract */

		nextlexblank(); /* skip over the operator */
		value = value - eval();
		goto more;

	}
	if (line[lexstart] == '^') { /* multiply */

		nextlexblank(); /* skip over the operator */
		value = value * eval();
		goto more;

	}
	if (line[lexstart] == '%') { /* divide */

		nextlexblank(); /* skip over the operator */
		value = value / eval();
		goto more;

	}
	if (line[lexstart] == '&') { /* and */

		nextlexblank(); /* skip over the operator */
		value = value & eval();
		goto more;

	}
	if (line[lexstart] == '!') { /* or */

		nextlexblank(); /* skip over the operator */
		value = value | eval();
		/* OPTIONAL PATCH 2 -- change to (value << 6) ! eval() */
		goto more;

	}

	if (isend(line[lexstart])) { 
		return value;
	}
	if (line[lexstart] == '/') { 
		return value;
	}
	if (line[lexstart] == ';') { 
		return value;
	}
	if (line[lexstart] == ')') { 
		return value;
	}
	if (line[lexstart] == ']') { 
		return value;
	}
	if (line[lexstart] == '<') { 
		return value;
	}

	error("expression");
	return 0;
}

int getexprs()
/* or together a list of blank-separated expressions, from the current
   lexeme onward, leave the current lexeme as the one after the last in
   the list!
*/
{
	int value;
	value = getexpr();

more:	/* here, we check if we are done */

	if (isdone(line[lexstart])) { /* no operand */
		return value;
	}
	if (line[lexstart] == ')') { /* end of list */
		return value;
	}
	if (line[lexstart] == ']') { /* end of list */
		return value;
	}

	{ /* interpret space as logical or */
		int temp;

		temp = getexpr();

		if (value <= 07777) { /* normal 12 bit value */
			value = value | temp;
		} else if (temp > 07777) { /* or together MRI opcodes */
			value = value | temp;
		} else if (temp < 0200) { /* page zero MRI */
			value = value | temp;
		} else if (   (((lc + reloc) & 07600) <= temp)
			   && (temp <= ((lc + reloc) | 00177))
			  ) {
			/* current page MRI */
			value = value | 00200 | (temp & 00177);
		} else {
			/* off page MRI */
			int loc;
			error("off page");

			/* having complained, fix it up */
			loc = 00177;
			while ((loc > cplc) && (cp[loc] != temp)) {
				loc--;
			}
			if (loc == cplc) {
				cp[cplc] = temp;
				cplc--;
			}
			value = value | 00600 | loc;
		}
		goto more;
	}
}

onepass()
/* do one assembly pass */
{
	lc = 0;
	reloc = 0;
	field = 0;
	cplc = 00177; /* points to end of page for [] operands */
	pzlc = 00177; /* points to end of page for () operands */
	radix = 8;
	listed = 1;
	lineno = 0;

getline:
	readline();
	nextlex();

restart:
	if (line[lexstart] == '/') {
		goto getline;
	}
	if (isend(line[lexstart])) {
		goto getline;
	}
	if (line[lexstart] == ';') {
		nextlex();
		goto restart;
	}
	if (line[lexstart] == '$') {
		putcp();
		putpz(); /* points to end of page for () operands */
		listline(); /* if it hasn't been listed yet, list it! */
		return;
	}
	if (line[lexstart] == '*') {
		int newlc;
		nextlex(); /* skip * (set origin symbol) */
		newlc = getexpr() & 07777;
		if ((newlc & 07600) != (lc & 07600)) { /* we changed pages */
			putcp();
		}
		lc = newlc - reloc;
		if (rimflag != 1) { /* put out origin if not in rim mode */
			if (obj != NULL) {
				putorg( lc );
			}
		}
		goto restart;
	}
	if (line[lexterm] == ',') {
		if (isalpha(line[lexstart])) {
			deflex( lexstart, lexterm, (lc + reloc) & 07777 );
		} else {
			error("label");
		}
		nextlex(); /* skip label */
		nextlex(); /* skip comma */
		goto restart;
	}
	if (line[lexterm] == '=') {
		if (isalpha(line[lexstart])) {
			int start = lexstart;
			int term = lexterm;
			nextlex(); /* skip symbol */
			nextlex(); /* skip trailing = */
			deflex( start, term, getexprs() );
		} else {
			error("symbol");
			nextlex(); /* skip symbol */
			nextlex(); /* skip trailing = */
			(void) getexprs(); /* skip expression */
		}
		goto restart;
	}
	if (isalpha(line[lexstart])) {
		int val;
		val = evalsym();
		if (val > 037777) { /* pseudo op */
			nextlex(); /* skip symbol */
			val = val & 07777;
			switch (val) {
			case 0: /* DECIMAL */
				radix = 10;
				break;
			case 1: /* OCTAL */
				radix = 8;
				break;
			case 2: /* ZBLOCK */
				val = getexpr();
				if (val < 0) {
					error("too small");
				} else if (val+lc-1 > 07777) {
					error("too big");
				} else {
					for ( ;val > 0; val--) {
						putout( lc, 0 );
						lc++;
					}
				}
				break;
			case 3: /* PAGE */
				putcp();
				if (isdone(line[lexstart])) { /* no arg */
					lc = (lc & 07600) + 00200;
                                } else {
                                        val = getexpr();
					lc = (val & 037) << 7;
                                }
				if (rimflag != 1) {
					if (obj != NULL) {
						putorg( lc );
					}
				}
				break;
			case 4: /* TEXT */
				{
					char delim = line[lexstart];
					int pack = 0;
					int count = 0;
					int index = lexstart + 1;
					while ((line[index] != delim) &&
					       !isend(line[index])       ) {
						pack = (pack << 6)
						     | (line[index] & 077);
						count++;
						if (count > 1) {
							putout( lc, pack );
							lc++;
							count = 0;
							pack = 0;
						}
						index++;
					}
					if (count != 0) {
						putout( lc, pack << 6 );
						lc++;
					} else { /* OPTIONAL PATCH 4 */
						putout( lc, 0 );
						lc++;
					}
					if (isend(line[index])) {
						lexterm = index;
						pos = index;
						error("parens");
						nextlex();
					} else {
						lexterm = index + 1;
						pos = index + 1;
						nextlex();
					}
				}
				break;
			case 5: /* EJECT */
				if (lst != NULL) {
					/* this will do for now */
					fprintf( lst, "\n . . . \n\n" );
					goto getline;
				}
				break;
			case 6: /* FIELD */
				putcp();
				putpz();
				if (isdone(line[lexstart])) {
					/* blank FIELD directive */
					val = field + 1;
				} else {
					/* FIELD with an argument */
					val = getexpr();
				}
				if (rimflag == 1) { /* can't change fields */
					error("rim mode");
				} else if ((val > 7) || (val < 0)) {
					error("field");
				} else if (obj != NULL) { /* change field */
		/*	b.baehr	patch	puto( ((val & 0007)<<3) | 0300 ); */
					fputc( ((val & 0007)<<3) | 0300, obj );
					field = val;
				}
				lc = 0;
				/* OPTIONAL PATCH 4 -- delete next line */
				lc = 0200;
				if (rimflag != 1) {
					if (obj != NULL) {
						putorg( lc );
					}
				}
				break;
			case 7: /* NOPUNCH */
				obj = NULL;
				break;
			case 010: /* ENPUNCH */
				obj = objsave;
				break;
			case 011: /* XLIST */
				if (isdone(line[lexstart])) {
					/* blank XLIST directive */
					FILE *temp = lst;
					lst = lstsave;
					lstsave = temp;
				} else {
					/* XLIST with an argument */
					if (getexpr() == 0) { /* ON */
						if (lst == NULL) {
							lst = lstsave;
							lstsave = NULL;
						}
					} else { /* OFF */
						if (lst != NULL) {
							lstsave = lst;
							lst = NULL;
						}
					}
				}
				break;
			case 012: /* IFZERO */
				if (getexpr() == 0) {
					condtrue();
				} else {
					condfalse();
				}
				break;
			case 013: /* IFNZRO */
				if (getexpr() == 0) {
					condfalse();
				} else {
					condtrue();
				}
				break;
			case 014: /* IFDEF */
				if (isalpha(line[lexstart])) {
					int val = evalsym();
					nextlex();
					if (val >= 0) {
						condtrue();
					} else {
						condfalse();
					}
				} else {
					error( "identifier" );
				}
				break;
			case 015: /* IFNDEF */
				if (isalpha(line[lexstart])) {
					int val = evalsym();
					nextlex();
					if (val >= 0) {
						condfalse();
					} else {
						condtrue();
					}
				} else {
					error( "identifier" );
				}
				break;
			case 016: /* RELOC */
				if (isdone(line[lexstart])) {
					/* blank RELOC directive */
					reloc = 0;
				} else {
					/* RELOC with an argument */
					val = getexpr();
					reloc = val - (lc + reloc);
				}
				break;
			case 017: /* SEGMNT */
				putcp();
				if (isdone(line[lexstart])) { /* no arg */
					lc = (lc & 06000) + 02000;
                                } else {
                                        val = getexpr();
					lc = (val & 003) << 10;
                                }
				if (rimflag != 1) {
					if (obj != NULL) {
						putorg( lc );
					}
				}
				break;
			case 020: /* BANK   */
				error("unsupported");
				/* should select a different 32K out of 128K */
				break;
			case 021: /* FIXMRI */
				if ((line[lexterm] == '=')
				&& (isalpha(line[lexstart]))) {
					int start = lexstart;
					int term = lexterm;
					nextlex(); /* skip symbol */
					nextlex(); /* skip trailing = */
					deflex( start, term, getexprs() );
				} else {
					error("symbol");
					nextlex(); /* skip symbol */
					nextlex(); /* skip trailing = */
					(void) getexprs(); /* skip expr */
				}
				break;
			} /* end switch */
			goto restart;
		} /* else */
			/* identifier is not pseudo op */
		/* } end if */
		/* fall through here if ident is not pseudo-op */
	}
	{ /* default -- interpret line as load value */
		putout( lc, getexprs() & 07777); /* interpret line load value */
		lc++;
		goto restart;
	}
}


/* main program */
main(argc, argv)
int argc;
char *argv[];
{
	getargs(argc, argv);

	onepass();

	rewind(in);
	obj = fopen(objname, "w"); /* must be "wb" under DOS */
	objsave = obj;
	lst = fopen(lstname, "w");
	lstsave = NULL;
	putleader();
	errors = 0;
	cksum = 0;

	onepass();

	if (lst == NULL) { /* undo effects of XLIST for any following dump */
		lst = lstsave;
	}
	if (dumpflag != 0) dump();
	obj = objsave; /* undo effects of NOPUNCH for any following checksum */
	if ((rimflag == 0) && (obj != NULL)) { /* checksum */
		/* for now, put out the wrong value */
		fputc( (cksum >> 6) & 0077, obj );
		fputc( cksum & 0077, obj );
	}
	putleader();
	exit (errors != 0);
}
