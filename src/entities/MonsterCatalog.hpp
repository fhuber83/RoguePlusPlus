#pragma once

/*
 * Monsters: choosing which kind shows up, making one, wandering monsters,
 * waking one up, and finding the one on a square.
 */

#include <array>
#include <string_view>

#include "core/Coord.hpp"
#include "core/Dice.hpp"
#include "core/Maybe.hpp"
#include "entities/Creature.hpp"
#include "entities/Stats.hpp"

namespace rogue {

struct Creature;

namespace entities {

/*
 * A kind of monster, one per letter (was struct monster)
 */
struct MonsterKind {
	std::string_view name;		/* What to call the monster */
	int carry;			/* Probability of carrying something */
	CreatureFlags flags;		/* Things about the monster */
	Stats stats;		/* Initial stats */
};

// The kinds of monster, by letter: monsters[letter - 'A']
extern const std::array<MonsterKind, 26> monsters;

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
void new_monster(Creature &tp, unsigned char type, Coord cp);

/*
 * f_restor:
 *	Restore the initial damage of flytraps.
 */
void f_restor();

/*
 * flytrap_attacks:
 *	What a venus flytrap does in a fight after hits hits (Player::fung_hit).
 */
rogue::Attacks flytrap_attacks(int hits);

/*
 * wanderer:
 *	Create a new wandering monster and aim it at the player.
 */
void wanderer();

/*
 * give_pack:
 *	Give a pack to a monster if it deserves one.
 */
void give_pack(Creature &tp);

/*
 * wake_monster:
 *	What to do when the hero steps next to a monster.
 */
Maybe<Creature> wake_monster(int y, int x);

}  // namespace entities
}  // namespace rogue
