#pragma once

/*
 * Making a new level: rooms, passages and mazes, the stairs, the things
 * and monsters on it, and sometimes a treasure room.
 *
 * Included by rogue.h after the legacy types.
 */

namespace rogue::world {

/*
 * new_level:
 *	Dig and draw a new level.
 */
void new_level();

/*
 * rnd_room:
 *	Pick a room that is really there.
 */
int rnd_room();

}  // namespace rogue::world
