/*
 * Defines for things used in mach_dep.c
 *
 * @(#)extern.h	5.1 (Berkeley) 5/11/82
 */

/*
 * Also the standard library includes, with a few libc "overrides" that the
 * original code relies on, and the functions of mach_dep.cpp.
 */

#ifndef EXTERN_H
#define EXTERN_H

/*
 * Set SUSv4 compatibility (POSIX.1-2008 base and XSI extensions)
 *
 * Enable setenv() and getenv(), remove toascii() and isascii()
 * Enable wide-character support ("complex renditions") in curses, if available
 * Implicitely set _POSIX_C_SOURCE 200809L
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html
 * https://man7.org/linux/man-pages/man7/feature_test_macros.7.html
 * https://www.gnu.org/software/libc/manual/html_node/Feature-Test-Macros.html
 */
#undef  _XOPEN_SOURCE  // set to 600 by pkg-config ncurses{,w}
#define _XOPEN_SOURCE 700


// uintptr_t, uint16_t
#include <stdint.h>

// std::format() for fatal(), before the "overrides" below
#include <format>
#include <string_view>
#include <utility>

// is{alpha,digit,upper,...}() and to{upper,lower,...}() families
#include <ctype.h>
#ifndef isascii
// Marked obsolescent in POSIX-2008, so not in <ctype.h> if C99 is used
#define isascii(c)	(((c) & ~0x7f) == 0)
#endif

// str{len,cat,cpy,cmp,chr}() and possibly others
#include <string.h>
#define bcopy(dest,source)	memmove(&(dest),&(source),sizeof(dest))
#define stpchr	strchr
#define setmem(dest,length,ch)	memset(dest,ch,length)

// f{open,read,seek,write,close}(), remove(), putchar()
#include <stdio.h>

// exit(), atoi(), NULL, EXIT_*, malloc(), free(), abs(), setenv(), getenv()
#include <stdlib.h>

// errno
#include <errno.h>

// pause(), access(), sleep(), close()
#include <unistd.h>
#define access(f)	access(f, F_OK)

// time(), nanosleep()
#include <time.h>

// setlocale()
#include <locale.h>

#ifndef TRUE
#define TRUE 	1
#define FALSE	0
#endif

#define msleep(ms)	md_nanosleep(1000000L * ms)

#ifdef __GNUC__
// for dummy arguments in stub functions
#define UNUSED(arg) __attribute__((unused))arg
#else
#define UNUSED(arg) arg
#endif


/*
 * Simplified version of <time.h> struct tm, to wrap and abstract it,
 * so local time source is opaque and easily replaceable.
 */
struct md_tm {
	int second;		/* Seconds	[0-60] (1 leap second) */
	int minute;		/* Minutes	[0-59] */
	int hour;		/* Hours	[0-23] */
	int day;		/* Day		[1-31] */
	int month;		/* Month	[0-11] */
	int year;		/* Year */
};
typedef struct md_tm TM;

/*
 * Function types
 */
void	setup(), flush_type(), credits(), start_terminal();
char	*newmem(unsigned int nbytes);
unsigned char	readchar();
unsigned char	swap_bits(unsigned char data, unsigned i, unsigned j, unsigned width);
long	md_time(void);
TM  	*md_localtime(void);
void	md_nanosleep(long nanoseconds);

// fatal() takes a std::format string and prints the text after closing the terminal
void	fatal_text(std::string_view text);

template <class... Args>
void
fatal(std::format_string<Args...> fmt, Args &&...args)
{
	fatal_text(std::format(fmt, std::forward<Args>(args)...));
}

void	md_exit(int status);

#endif //EXTERN_H
