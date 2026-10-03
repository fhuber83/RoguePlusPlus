/*
 * Reading the player's keys.
 *
 * mach_dep.c	1.4 (A.I. Design) 12/1/84
 */

#include "game/Keyboard.hpp"

#include <string>

#include "core/Glyphs.hpp"
#include "game/Game.hpp"
#include "game/StatusLine.hpp"
#include "ui/Display.hpp"
#include "ui/Input.hpp"
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
	if (std::string &typeahead = game().turn.typeahead; !typeahead.empty()) {
		SIG2();
		ui::display().flush();
		unsigned char ch = typeahead.front();
		typeahead.erase(0, 1);
		return ch;
	}
	/*
	 * while there are no characters in the type ahead buffer
	 * update the status line at the bottom of the screen
	 */
	int xch;
	do
	{
		SIG2();  /* Rogue spends a lot of time here */
		ui::display().flush();
	}
	while ((xch = ui::input().read_key(250)) == ui::key::None);
	unsigned char ch = ui::command_char(xch);
	if (ch == ESCAPE)
		game().turn.count = 0;
	return ch;
}

}  // namespace rogue
