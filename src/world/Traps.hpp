#pragma once

#include <string_view>

#include "core/Coord.hpp"
#include "world/Trap.hpp"

/*
 * What traps do: springing one, and falling to the next level (a trapdoor,
 * or the floor giving way under a rogue in no room).
 */

namespace rogue::world {

/*
 * tr_name:
 *	The name of a trap, with its article ("a beartrap"). An unknown kind
 *	says so on the message line and has the name "".
 */
std::string_view tr_name(Trap type);

/*
 * be_trapped:
 *	The rogue stepped on the trap at tc: it is shown, and it does what it
 *	does. Returns its kind.
 */
Trap be_trapped(Coord tc);

/*
 * descend:
 *	The rogue falls to the next level, with mesg on the message line
 *	(" " if it is empty), and may be hurt by the fall.
 */
void descend(std::string_view mesg);

}  // namespace rogue::world
