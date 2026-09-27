#include <cctype>
#include <cstring>

// isascii() is obsolescent in POSIX-2008
static bool is_ascii(char ch) { return (ch & ~0x7f) == 0; }

// Locale-independent versions, as expected by Rogue
bool is_alpha(char ch) { return (is_ascii(ch) && isalpha(ch)); }
bool is_upper(char ch) { return (is_ascii(ch) && isupper(ch)); }
bool is_lower(char ch) { return (is_ascii(ch) && islower(ch)); }
bool is_digit(char ch) { return (is_ascii(ch) && isdigit(ch)); }
bool is_space(char ch) { return (is_ascii(ch) && isspace(ch)); }
bool is_print(char ch) { return (is_ascii(ch) && isprint(ch)); }

/*
 * Copy at most count characters and terminate. Not strncpy(), which pads
 * and may not terminate.
 */
char *
stccpy(char *s1, char *s2, int count)
{
	while (count-->0 && *s2)
		*s1++ = *s2++;
	*s1 = 0;
	/*
	 * lets return the address of the end of the string so
	 * we can use that info if we are going to cat on something else!!
	 */
	return (s1);
}


/*
 * redo Lattice token parsing routines
 */

// strip leading blanks
char *
stpblk(char *str)
{
	while (is_space(*str))
		str++;
	return(str);
}

/*
 * remove trailing whitespace from the end of a line
 */
char *
endblk(char *str)
{
	char *backup;

	backup = str + strlen(str);
	while (backup != str && is_space(*(--backup)))
		*backup = 0;
	return(str);
}

/*
 * lcase: convert a string to lower case
 */
void
lcase(char *str)
{
	while ( (*str = tolower(*str)) )
		str++;
}
