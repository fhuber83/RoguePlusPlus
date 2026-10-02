#pragma once

#include "core/Coord.hpp"

namespace rogue {
struct Creature;
}

/*
 * The rogue's moves: one step (fighting, picking up, springing traps and
 * turning corners of passages on the way), starting a run, and the random
 * step of a confused rogue or monster.
 */

namespace rogue {

/*
 * do_run:
 *	Start the rogue running in the direction of key ch.
 */
void do_run(unsigned char ch);

/*
 * do_move:
 *	Move the rogue by dy, dx if the move is legal, and handle what he
 *	moves into (a monster to fight, a trap, a door, something to pick up).
 *	A running rogue follows a passage around a corner.
 */
void do_move(int dy, int dx);

/*
 * rndmove:
 *	A random step for a confused rogue or monster: a square next to it,
 *	or where it stands if that square can't be stepped on.
 */
Coord rndmove(const Creature &who);

}  // namespace rogue
