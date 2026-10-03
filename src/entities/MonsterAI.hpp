#pragma once

/*
 * How monsters move: chasing the rogue or what they want, whether the rogue
 * can see them, and slimes dividing.
 */

#include <optional>

#include "core/Coord.hpp"
#include "core/Maybe.hpp"
#include "entities/Creature.hpp"

namespace rogue {

struct Creature;

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
void start_run(Coord runner);

/*
 * see_monst:
 *	Return true if the hero can see the monster.
 */
bool see_monst(const Creature &mp);

/*
 * find_dest:
 *	Find the proper destination for the monster.
 */
Destination find_dest(const Creature &tp);

/*
 * slime_split:
 *	Called when it has been decided that a slime should divide itself.
 */
void slime_split(Creature &tp);

/*
 * plop_monster:
 *	Pick a spot around (r, c) for a new monster to appear, or nullopt if
 *	there is none.
 */
std::optional<Coord> plop_monster(int r, int c);

/*
 * aggravate:
 *	Aggravate all the monsters on this level: each starts running.
 */
void aggravate();

}  // namespace entities
}  // namespace rogue
