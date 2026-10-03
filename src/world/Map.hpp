#pragma once

/*
 * The squares of the level map: where a square is kept, whether it is on
 * the map, and what stands, lies or can be stepped on there.
 */

#include "core/Maybe.hpp"

namespace rogue {
struct Item;
}

namespace rogue::world {

/*
 * INDEX:
 *	The index of a map square in Level::map and Level::flags.
 */
int INDEX(int y, int x);

/*
 * offmap:
 *	Whether a square is off the map (the message and status lines, or
 *	past the edge of the screen).
 */
bool offmap(int y, int x);

/*
 * winat:
 *	What the rogue would see at a square: a monster's disguise, or the map.
 */
unsigned char winat(int y, int x);

/*
 * step_ok:
 *	Whether it is ok to step on a square showing ch: not a wall, not blank,
 *	not a monster.
 */
bool step_ok(unsigned char ch);

/*
 * find_obj:
 *	The object lying on the floor at y, x, if any.
 */
Maybe<Item> find_obj(int y, int x);

}  // namespace rogue::world
