#pragma once

/*
 * Rooms at play time: entering and leaving one, which room a square is
 * in, and what the rogue can see from where it stands.
 */

#include <optional>

#include "core/Coord.hpp"
#include "world/RoomRef.hpp"

namespace rogue::world {

inline constexpr int LAMPDIST = 3;	/* squared distance he sees in the dark (next to him) */

struct Room;

/*
 * roomin:
 *	Find what room some coordinates are in. Null means they aren't in any
 *	room.
 */
std::optional<RoomRef> roomin(Coord cp);

/*
 * cansee:
 *	Returns true if the hero can see a certain coordinate.
 */
bool cansee(int y, int x);

/*
 * rnd_pos:
 *	Pick a random spot in a room.
 */
Coord rnd_pos(const Room &rp);

/*
 * enter_room:
 *	Code that is executed whenever you appear in a room.
 */
void enter_room(Coord cp);

/*
 * leave_room:
 *	Code for when we exit a room.
 */
void leave_room(Coord cp);

/*
 * teleport:
 *	Move the rogue to a random square of a random room, which confuses
 *	him (a scroll of teleportation, a teleport trap).
 */
void teleport();

}  // namespace rogue::world
