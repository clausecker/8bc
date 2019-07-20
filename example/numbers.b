main()
{
	extrn print8, putchar;
	auto i 0;

	while (i < 100) {
		print8(i++);
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
