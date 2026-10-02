#pragma once

/*
 * What the special keys mean to the game: the PC's arrow and function keys
 * as the command characters they stand for.
 */

namespace rogue::ui {

/*
 * command_char:
 *	The character the game reads for a key from Input::read_key(): an
 *	arrow or keypad key is its move (Home is 'y', ...), F1 to F9 and
 *	Alt-F9 the commands on the help screen, Insert '>', Delete 's', keypad
 *	Enter '\n' and Backspace 'h'. Any other key is its own character.
 */
unsigned char command_char(int key);

}  // namespace rogue::ui
