/* echo.b -- echo input until ^D is pressed */
main()
{
	extrn getchar, putchar;
	auto c;

	while ((c = getchar()) != '*e')
		putchar(c);
}
