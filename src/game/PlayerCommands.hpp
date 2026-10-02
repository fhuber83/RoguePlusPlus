#pragma once

#include <optional>
#include <string>

#include "core/Coord.hpp"

/*
 * Commands and prompts of the rogue that belong to no other module: the
 * direction prompt, the stairs, and defining the F9 macro.
 */

namespace rogue {

/*
 * get_dir:
 *	Ask for a direction and put it in game().turn.delta (a confused rogue
 *	may get another one). False if the rogue pressed Escape. Repeating a
 *	command (turn.again) keeps the last direction.
 */
bool get_dir();

/*
 * find_dir:
 *	The direction a key stands for, or nullopt if it is none.
 */
std::optional<Coord> find_dir(unsigned char ch);

/*
 * d_level:
 *	The rogue wants to go down a level.
 */
void d_level();

/*
 * u_level:
 *	The rogue wants to go up a level: only with the amulet, and out of the
 *	dungeon from level 1.
 */
void u_level();

/*
 * do_macro:
 *	Prompt the player for the definition of the F9 macro.
 */
void do_macro(std::string &macro);

}  // namespace rogue
