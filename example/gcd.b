/* gcd.b -- compute greatest common divisors with the algorithm of Stein */

/* print a number to the terminal */
putnum(i)
{
	extrn putchar;

	putchar('0' + (i >> 9));
	putchar('0' + (i >> 6 & 7));
	putchar('0' + (i >> 3 & 7));
	putchar('0' + (i & 7));
}

/* read a number from the terminal */
getnum(base)
{
	extrn getchar;
	auto c, n 0;

	while ((c = getchar() - '0') < base)
		n = n * base + c;

	return (n);
}
	

/* print a *e terminated string */
puts(s)
{
	extrn putchar;

	while (*s != '*e')
		putchar(*s++);
}

gcd(a, b)
{
	auto k 0;

	while (a != b) {
		if (~a & 1) {
			a =>> 1;
			if (~b & 1) {
				b =>> 1;
				++k;
			}
		} else if (~b & 1)
			b =>> 1;
		else if (a > b)
			a = a - b >> 1;
		else {
			auto tmp;

			tmp = a;
			a = b - a >> 1;
			b = tmp;
		}
	}

	return (a << k);
}

main() {
	extrn putchar, puts, putnum, getnum, gcd;

	auto a, b, q;

	while (1) {
		puts("A = ");
		a = getnum(8);
		if (!a)
			return;

		puts("B = ");
		b = getnum(8);
		if (!b)
			return;

		puts("GCD(A,B) = ");
		putnum(gcd(a, b));
		putchar('*n');
	}
}
