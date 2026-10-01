#pragma once

/*
 * Monsters: choosing which kind shows up, making one, wandering monsters,
 * waking one up, and finding the one on a square.
 *
 * Included by rogue.h after entities/Creature.hpp (Creature, CreatureFlags)
 * and the legacy struct stats.
 */

namespace rogue {

class Creature;

namespace entities {

/*
 * A kind of monster, one per letter (was struct monster)
 */
struct MonsterKind {
	std::string_view m_name;		/* What to call the monster */
	int m_carry;			/* Probability of carrying something */
	CreatureFlags m_flags;		/* Things about the monster */
	struct stats m_stats;		/* Initial stats */
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

/*
 * moat:
 *	The monster at a coordinate, or null if there is none.
 */
Maybe<Creature> moat(int my, int mx);

}  // namespace entities
}  // namespace rogue
