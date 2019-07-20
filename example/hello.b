main()
{
	extrn putchar;
	auto str "hello, world*n";

	while (*str != '*e')
		putchar(*str++);
}
