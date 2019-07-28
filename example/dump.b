/* memory dumping program */

/* print a number to the terminal */
putnum(i)
{
	extrn putchar;

	putchar('0' + (i >> 9));
	putchar('0' + (i >> 6 & 7));
	putchar('0' + (i >> 3 & 7));
	putchar('0' + (i & 7));
}

main() {
	extrn putchar, putnum;
	auto i 0, j;

again:	putnum(i);
	putchar(':');

	j = 0;
	while (j < 010) {
		putchar(' ');
		putnum(i[j++]);
	}

	putchar('*n');

	i =+ 010;
	if (i != 0)
		goto again;
}
