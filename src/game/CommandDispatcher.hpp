#pragma once

#include "game/Command.hpp"

/*
 * Reading and carrying out the player's commands: count and prefix keys
 * (digits, f, g, a, Escape), repeating and running, then one Command per
 * turn (see game/Command.hpp for the keys).
 */

namespace rogue {

/*
 * command:
 *	One turn of the rogue (two or three when hasted): a command, or a
 *	turn lost while he can't move, then the fuses, the daemons and the
 *	searching and teleport rings.
 */
void command();

/*
 * execcom:
 *	Read and carry out commands until one that uses up a turn.
 */
void execcom();

/*
 * show_count:
 *	Show the count prefix being typed.
 */
void show_count();

/*
 * resume_saved_game:
 *	A restored game goes on where it was saved: in the middle of a turn,
 *	waiting for a key. The next command() and get_prefix() then skip what
 *	was done before the save (the haste roll, look(TRUE)), which could roll
 *	dice again and make the game go differently.
 */
void resume_saved_game();

}  // namespace rogue
