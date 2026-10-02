/*
 * Starting the terminal and leaving the program.
 *
 * mach_dep.c	1.4 (A.I. Design) 12/1/84
 */

#include "platform/Session.hpp"

#include <cstdio>
#include <cstdlib>

#include "core/Config.hpp"
#include "ui/Display.hpp"

namespace rogue::platform {

void
start_terminal(bool monochrome)
{
	if (auto started = ui::start_terminal(monochrome); !started)
		fatal("{}", started.error());
}

void
fatal_text(std::string_view text)
{
	ui::stop_terminal();

	std::fwrite(text.data(), 1, text.size(), stdout);
	md_exit(EXIT_SUCCESS);
}

void
md_exit(int status)
{
	ui::stop_terminal();
	if constexpr (config::debug_checks)
		std::fputs("Exited normally\n", stdout);
	std::exit(status);
}

}  // namespace rogue::platform
