#pragma once

/*
 * Rooms at play time: entering and leaving one, which room a square is
 * in, and what the rogue can see from where it stands.
 *
 * Included by rogue.h after the legacy types (coord, struct room).
 */

struct room;

namespace rogue::world {

/*
 * roomin:
 *	Find what room some coordinates are in. Null means they aren't in any
 *	room.
 */
struct room *roomin(coord *cp);

/*
 * diag_ok:
 *	Check to see if the move is legal if it is diagonal.
 */
bool diag_ok(coord *sp, coord *ep);

/*
 * cansee:
 *	Returns true if the hero can see a certain coordinate.
 */
bool cansee(int y, int x);

/*
 * rnd_pos:
 *	Pick a random spot in a room.
 */
void rnd_pos(struct room *rp, coord *cp);

/*
 * enter_room:
 *	Code that is executed whenever you appear in a room.
 */
void enter_room(coord *cp);

/*
 * leave_room:
 *	Code for when we exit a room.
 */
void leave_room(coord *cp);

}  // namespace rogue::world
