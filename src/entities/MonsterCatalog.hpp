#pragma once

/*
 * Monsters: choosing which kind shows up, making one, wandering monsters,
 * waking one up, and finding the one on a square.
 *
 * Included by rogue.h after extern.h (byte) and entities/Creature.hpp
 * (Creature).
 */

namespace rogue {

class Creature;

namespace entities {

/*
 * randmonster:
 *	Pick a monster to show up. The lower the level, the meaner the
 *	monster.
 */
char randmonster(bool wander);

/*
 * pick_mons:
 *	Choose a sort of monster for the enemy of a vorpally enchanted weapon.
 */
char pick_mons();

/*
 * new_monster:
 *	Pick a new monster and add it to the list.
 */
void new_monster(Creature *tp, byte type, coord *cp);

/*
 * f_restor:
 *	Restore the initial damage string for flytraps.
 */
void f_restor();

/*
 * wanderer:
 *	Create a new wandering monster and aim it at the player.
 */
void wanderer();

/*
 * give_pack:
 *	Give a pack to a monster if it deserves one.
 */
void give_pack(Creature *tp);

/*
 * wake_monster:
 *	What to do when the hero steps next to a monster.
 */
Creature *wake_monster(int y, int x);

/*
 * moat:
 *	The monster at a coordinate, or null if there is none.
 */
Creature *moat(int my, int mx);

}  // namespace entities
}  // namespace rogue
