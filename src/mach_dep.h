#pragma once

/*
 * The machine-dependent functions of mach_dep.cpp: time, the terminal, keys,
 * the title screen and exiting. (Was the second half of extern.h.)
 */

#include <format>
#include <string_view>
#include <utility>

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

void	setup(), flush_type(), credits(), start_terminal();
unsigned char	readchar();
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
