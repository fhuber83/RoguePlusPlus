/*
 * Reading the player's keys.
 *
 * mach_dep.c	1.4 (A.I. Design) 12/1/84
 */

#include "rogue.h"
#include "ui/Keys.hpp"

namespace rogue {

/*
 * flush_type:
 *	Flush typebuf for traps, etc.
 */
void
flush_type()
{
	game().turn.typeahead.clear();
}

/*
 * readchar:
 *	Return the next input character, from the macro or from the keyboard.
 */
unsigned char
readchar()
{
	int xch;
	unsigned char ch;

	if (std::string &typeahead = game().turn.typeahead; !typeahead.empty()) {
		SIG2();
		display().flush();
		ch = typeahead.front();
		typeahead.erase(0, 1);
		return ch;
	}
	/*
	 * while there are no characters in the type ahead buffer
	 * update the status line at the bottom of the screen
	 */
	do
	{
		SIG2();  /* Rogue spends a lot of time here */
		display().flush();
	}
	while ((xch = input().read_key(250)) == ui::key::None);
	ch = ui::command_char(xch);
	if (ch == ESCAPE)
		game().turn.count = 0;
	return ch;
}

}  // namespace rogue
