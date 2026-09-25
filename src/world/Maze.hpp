#pragma once

/*
 * Maze rooms, dug while a new level is made.
 *
 * Included by rogue.h after the legacy types (struct room).
 */

struct room;

namespace rogue::world {

/*
 * draw_maze:
 *	Dig a maze into the room's area.
 */
void draw_maze(struct room *rp);

}  // namespace rogue::world
