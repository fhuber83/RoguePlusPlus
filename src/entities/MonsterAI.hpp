#pragma once

/*
 * How monsters move: chasing the rogue or what they want, whether the rogue
 * can see them, and slimes dividing.
 *
 * Included by rogue.h after the legacy types (coord) and
 * entities/Creature.hpp (Creature).
 */

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
