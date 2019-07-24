/* multiplication test program */

/* print a number to the terminal */
putnum(i)
{
	extrn putchar;

	putchar('0' + (i >> 9));
	putchar('0' + (i >> 6 & 7));
	putchar('0' + (i >> 3 & 7));
	putchar('0' + (i & 7));
}

/* multiply a with b and return the product */
multiply(a, b)
{
	auto q 0;

	while (b) {
		if (b & 1)
			q =+ a;

		b =>> 1;
		a =+ a;
	}

	return (q);
}

/* print a *e terminated string */
puts(s)
{
	extrn putchar;

	while (*s != '*e')
		putchar(*s++);
}

main() {
	extrn putnum, multiply, putchar, puts;
	auto a 0, b 0, q_brt, q_mul;

	while (++a) {
		while (++b) {
			q_brt = a * b;
			q_mul = multiply(a, b);

			if (q_brt != q_mul) {
				puts("FAIL ");
				putnum(a);
				putchar(' ');
				putnum(b);
				putchar(' ');
				putnum(q_brt);
				putchar(' ');
				putnum(q_mul);
				putchar('*n');
			}
		}
	}
}
