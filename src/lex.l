%{
#include <ctype.h>

#include "y.tab.h"
%}

%x	COMMENT
ESC	\*[0et*'"n]

%%

"/*"			BEGIN COMMENT;
<COMMENT>"*/"		BEGIN INITIAL;
<COMMENT>.		/* discard comments */

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

[0-9]+			{ /* parse integer constant */ }
'([^*']|ESC)'		{ /* parse character constant */ }
\"([^*"]|ESC)*\"	{ /* parse string constant */ }
[A-Za-z_][[:alnum:]_]*	{
				size_t i;
				for (i = 0; yytext[i] != '\0'; i++)
					yytext[i] = toupper(yytext[i]);
				return (NAME);
			}
[[:space:]]		/* ignore white space */
\"			printf("unterminated string constant\n");
'			printf("unterminated character constant\n");
[!-?]			return (yytext[0]);
[`-~]			return (yytext[0] - 040);	/* convert to 6 bit ASCII */
.			printf("stray %03o\n", (unsigned char)yytext[0]);
