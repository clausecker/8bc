/* fibonacci.b -- print Fibonacci numbers */

main()
{
	extrn fib, print8, putchar;
	auto i 0;

	while (i <= 18) {
		print8(fib(i++));
		putchar('*n');
	}
}

print8(i)
{
	extrn putchar;

	putchar('0' + (i >> 9));
	putchar('0' + (i >> 6 & 7));
	putchar('0' + (i >> 3 & 7));
	putchar('0' + (i & 7));
}

fib(n)
{
	auto a 0, b 1, tmp;

	while (n-- > 0) {
		tmp = a;
		a = b;
		b =+ tmp;
	}

	return (a);
}
