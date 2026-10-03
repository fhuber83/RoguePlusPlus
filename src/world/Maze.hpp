#pragma once

/*
 * Maze rooms, dug while a new level is made.
 */

namespace rogue::world {

struct Room;

/*
 * draw_maze:
 *	Dig a maze into the room's area.
 */
void draw_maze(Room &rp);

}  // namespace rogue::world
