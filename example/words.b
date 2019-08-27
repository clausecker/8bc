/* count words in the input and print statistics */
main()
{
	extrn getword, findword, nwords, words, freqs, endbuf, putstats;
	auto word, i;

	while (word = getword()) {
		i = findword(word);
		if (i == nwords) {
			words[nwords] = word;
			freqs[nwords] = 1;
			++nwords;
		} else {
			++freqs[i];
			endbuf = word; /* discard word */
		}
	}

	putstats();
}

/*
 * Read a word from the terminal and append it to wordbuf.
 * Return 0 if EOF appeared before anything was read.
 * Otherwise return a pointer to the word.
 */
getword()
{
	extrn endbuf, getchar;
	auto word, p, c;

	word = p = endbuf;

	/* skip white space */
	while ((c = getchar()) != '*e')
		if (c != ' ' & c != '*n')
			goto done;

	/* if EOF hit, no word was scanned */
	return (0);

done:	*p++ = c;
	while ((c = getchar()) != '*e') {
		if (c == ' ' | c == '*n')
			goto end;

		*p++ = c;
	}

end:	*p++ = '*e';
	endbuf = p;
	return (word);
}

/* compare strings a and b for equality */
streq(a, b)
{
	while (*a != '*e') {
		if (*a++ != *b++)
			return 0;
	}

	return (*b == '*e');
}

/* find word in words, return nwords if not found */
findword(word)
{
	extrn nwords, words, streq;
	auto i 0;

	while (i < nwords) {
		if (streq(words[i], word))
			return (i);

		++i;
	}

	return (nwords);
}

/* print a string with a newline */
puts(s)
{
	extrn putchar;

	while (*s != '*e')
		putchar(*s++);

	putchar('*n');
}

/* print a number to the terminal */
putnum(i)
{
	extrn putchar;

	putchar('0' + (i >> 9));
	putchar('0' + (i >> 6 & 7));
	putchar('0' + (i >> 3 & 7));
	putchar('0' + (i & 7));
}

/* print word count statistics */
putstats()
{
	extrn nwords, putnum, freqs, putchar, puts, words;
	auto i 0;

	while (i < nwords) {
		putnum(freqs[i]);
		putchar(' ');
		puts(words[i++]);
	}
}

nwords 0;	/* the number of words found so far */
words[32];	/* words found */
freqs[32];	/* word frequencies */
endbuf end;	/* first free character in wordbuf */

