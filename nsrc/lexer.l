%{
#include <ctype.h>
#include <string.h>

#include "param.h"
#include "pdp8.h"
#include "error.h"
#include "parser.h"
#include "y.tab.h"

/* silence warning about unused yyunput() */
#define YY_NO_UNPUT YY_NO_UNPUT

static int nconst(void), cconst(void);
static int unescape(const char **);

/* the current line number */
unsigned short lineno = 1;
%}

ESC	\*[0et*'"n]

%%

%{
	/* default assignment if we don't have a better value */
	memset(yylval.name, '0', MAXNAME);
	yylval.value = TOKEN;
%}

"/*"			{
				char c0 = '\0', c1;

				/*
				 * While POSIX says that input() returns 0 on
				 * EOF, some lex versions return EOF instead.
				 */
				while (c1 = input(), c1 != 0 && c1 != EOF) {
					if (c1 == '\n')
						lineno++;

					if (c0 == '*' && c1 == '/')
						goto done;

					c0 = c1;
				}

				/* EOF encountered while in comment */
				error(NULL, "unterminated comment");

			done:	;
			}

\n			lineno++;
"=*"			return (ASMUL);
=%			return (ASMOD);
"=/"			return (ASDIV);
"=+"			return (ASADD);
=-			return (ASSUB);
=<<			return (ASSHL);
=>>			return (ASSHR);
=<			return (ASLT);
=>			return (ASGT);
=<=			return (ASLE);
=>=			return (ASGE);
===			return (ASEQ);
=!=			return (ASNE);
=&			return (ASAND);
"=^"			return (ASXOR);
=\\			return (ASOR);
"++"			return (INC);
--			return (DEC);
"<<"			return (SHL);
>>			return (SHR);
"<="			return (LE);
>=			return (GE);
==			return (EQ);
!=			return (NE);

[Aa][Uu][Tt][Oo]	return (AUTO);
[Ee][Xx][Tt][Rr][Nn]	return (EXTRN);
[Cc][Aa][Ss][Ee]	return (CASE);
[Ii][Ff]		return (IF);
[Ee][Ll][Ss][Ee]	return (ELSE);
[Ww][Hh][Ii][Ll][Ee]	return (WHILE);
[Ss][Ww][Ii][Tt][Cc][Hh]	return (SWITCH);
[Gg][Oo][Tt][Oo]	return (GOTO);
[Rr][Ee][Tt][Uu][Rr][Nn]	return (RETURN);
[Bb][Rr][Ee][Aa][Kk]	return (BREAK);
[Dd][Ee][Ff][Aa][Uu][Ll][Tt]	return (DEFAULT);

[0-9]+			yylval.value = nconst() | RCONST;  return (CONSTANT);
\'([^*'\n]|{ESC})\'	yylval.value = cconst() | RCONST;  return (CONSTANT);
\"([^*"\n]|{ESC})*\"	error(NULL, "string constants not implemented");

\"			error(NULL, "unterminated string constant\n");
'			error(NULL, "unterminated character constant\n");

[A-Za-z_][[:alnum:]_]*	{
				size_t i;
				for (i = 0; yytext[i] != '\0'; i++)
					yytext[i] = toupper(yytext[i]);


				if (i > MAXNAME)
					warn(yytext, "name truncated");

				strncpy(yylval.name, yytext, MAXNAME);

				return (NAME);
			}

[[:space:]]		/* ignore white space */

[!-^]			return (yytext[0]);
[`a-z{|}~]		return (yytext[0] - 040);	/* convert to 6 bit ASCII */

.			error(NULL, "stray %03o", (unsigned char)yytext[0]);

%%

/*
 * parse a numerical (decimal or octal) constant from yytext and return
 * its value.  As per specificiation, the digits 8 and 9 are legal even
 * in octal constants.
 */
static int
nconst(void)
{
	size_t i;
	int base = yytext[0] == '0' ? 8 : 10, n = 0;

	for (i = 0; yytext[i] != '\0'; i++)
		n = n * base + yytext[i] - '0';

	return (n & 07777);
}

/*
 * parse a character literal from yytext and return its value.
 */
static int
cconst(void)
{
	const char *s = yytext + 1;

	/* s[0] == '\'' is assumed */
	return (unescape(&s) & 07777);
}

/*
 * Substitute an escape sequence for the character it stands for.  If
 * *s does not point to an escape sequence, return the character it
 * points to.  Lastly, advance *s past whatever was just returned.
 */
static int
unescape(const char **s)
{
	int c;

	if (**s != '*')
		return (*(*s)++);

	*s += 2;
	c = (*s)[-1];

	switch (c) {
	case '0': return ('\0');
	case 'e': return (0004); /* ASCII EOF */
	case '(': return ('{');
	case ')': return ('}');
	case 't': return ('\t');
	case 'n': return ('\n');
	default:
		warn(NULL, "unrecognised escape sequence *%c", c);

		/* FALLTHROUGH */

	case '*':
	case '\'':
	case '\"':
		return (c);
	}
}