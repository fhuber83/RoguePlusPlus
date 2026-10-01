#pragma once

/*
 * Reading the player's keys: from the macro being played, or from the
 * keyboard while the clock ticks.
 */

namespace rogue {

/*
 * readchar:
 *	Return the next input character, from the macro or from the keyboard,
 *	with the special keys as their command characters (ui::command_char()).
 *	Escape cancels the repeat count.
 */
unsigned char readchar();

/*
 * flush_type:
 *	Forget what is left of the macro being played (for traps, etc.).
 */
void flush_type();

}  // namespace rogue
