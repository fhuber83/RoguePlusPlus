#pragma once

#include <format>
#include <string_view>
#include <utility>

/*
 * Starting the terminal, and the ways out of the program: each closes the
 * terminal first. (Was the rest of mach_dep.h.)
 */

namespace rogue::platform {

/*
 * start_terminal:
 *	Start the terminal (in black and white if monochrome), or exit with
 *	the reason it could not.
 */
void start_terminal(bool monochrome);

/*
 * fatal_text:
 *	Close the terminal, print text and exit. fatal() formats it.
 */
[[noreturn]] void fatal_text(std::string_view text);

/*
 * fatal:
 *	Exit with a message, a std::format string, printed after closing the
 *	terminal. Also the normal way out after a save or a quit.
 */
template <class... Args>
[[noreturn]] void
fatal(std::format_string<Args...> fmt, Args &&...args)
{
	fatal_text(std::format(fmt, std::forward<Args>(args)...));
}

/*
 * md_exit:
 *	The single point of exit for Rogue: close the terminal and exit with
 *	status.
 */
[[noreturn]] void md_exit(int status);

}  // namespace rogue::platform
