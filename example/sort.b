/* sort input lines alphabetically */
main() {
	extrn readall, mklines, sort, puts, end;
	auto i 0, nlines, input, lines;

	input = &end;
	lines = readall(input);
	nlines = mklines(lines, input);
	sort(lines, nlines);
	while (i < nlines)
		puts(lines[i++]);
}

/* read the input into buffer and return a pointer to its end */
readall(buffer)
{
	extrn getchar;

	while ((*buffer++ = getchar()) != '*e')
		;

	return (buffer);
}

/*
 * Split buffer into *e terminated lines.  Store pointers to the
 * lines into lines and return the number of lines.
 */
mklines(lines, buffer)
{
	auto i 0, nlines 0;

	while (1) {
		lines[nlines++] = buffer + i;

		while (buffer[i] != '*n' & buffer[i] != '*e')
			i++;

		if (buffer[i] == '*e')
			return (nlines);
		else
			buffer[i++] = '*e';
	}
}

/* compare strings alphanumerically.  Return -1, 0, or 1 */
strcmp(a, b)
{
	auto i 0;

	while (a[i] == b[i])
		if (a[i++] == '*e')
			return 0;

	return (a[i] > b[i]) - (a[i] < b[i]);
}

/* sort string pointers in lines by string order */
sort(lines, nlines)
{
	extrn strcmp;
	auto i, j 0, tmp;

	/* insertion sort */
	while (j < nlines) {
		i = j++;
		tmp = lines[i];

		while (i > 0 & strcmp(lines[i-1], tmp) == 1) {
			lines[i] = lines[i - 1];
			--i;
		}

		lines[i] = tmp;
	}
}

/* print string followed by a newline */
puts(str)
{
	extrn putchar;

	while (*str != '*e')
		putchar(str++);

	putchar('*n');
}
