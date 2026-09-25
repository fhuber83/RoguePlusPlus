#pragma once

/*
 * How monsters move: chasing the rogue or what they want, whether the rogue
 * can see them, and slimes dividing.
 *
 * roomin(), cansee() and diag_ok() are level geometry that the chase code
 * happened to hold; they belong with the level (phase 7.6).
 *
 * Included by rogue.h after the legacy types (coord, struct room) and
 * entities/Creature.hpp (Creature).
 */

struct room;

namespace rogue {

class Creature;

namespace entities {

/*
 * runners:
 *	Make all the running monsters move.
 */
void runners();

/*
 * start_run:
 *	Set a monster running after something or stop it from running (for
 *	when it dies).
 */
void start_run(coord *runner);

/*
 * see_monst:
 *	Return true if the hero can see the monster.
 */
bool see_monst(Creature *mp);

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
 * roomin:
 *	Find what room some coordinates are in. Null means they aren't in any
 *	room.
 */
struct room *roomin(coord *cp);

/*
 * find_dest:
 *	Find the proper destination for the monster.
 */
coord *find_dest(Creature *tp);

/*
 * slime_split:
 *	Called when it has been decided that a slime should divide itself.
 */
void slime_split(Creature *tp);

/*
 * plop_monster:
 *	Pick a spot around (r, c) for a new monster to appear, into cp.
 *	Returns false if there is none.
 */
bool plop_monster(int r, int c, coord *cp);

}  // namespace entities
}  // namespace rogue
