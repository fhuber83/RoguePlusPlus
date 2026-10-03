/*
 * File with various monster functions in it
 *
 * monsters.c	1.4 (A.I. Design)	12/14/84
 */

#include "entities/MonsterCatalog.hpp"

#include <array>
#include <optional>
#include <string_view>

#include "core/Coord.hpp"
#include "core/Dice.hpp"
#include "core/Glyphs.hpp"
#include "core/Maybe.hpp"
#include "entities/Creature.hpp"
#include "entities/MonsterAI.hpp"
#include "entities/Stats.hpp"
#include "game/Game.hpp"
#include "game/Messages.hpp"
#include "game/Pool.hpp"
#include "items/ItemCatalog.hpp"
#include "items/Kinds.hpp"
#include "rules/Combat.hpp"
#include "rules/Durations.hpp"
#include "rules/Scheduler.hpp"
#include "world/Level.hpp"
#include "world/LevelGenerator.hpp"
#include "world/Room.hpp"
#include "world/RoomRef.hpp"
#include "world/Rooms.hpp"

namespace rogue::entities {

/*
 * A value the game never reads (was ___): hp and max_hp of the monster
 * templates, as each new monster rolls its hit points.
 */
constexpr int NA = 1;
// Every monster's strength
constexpr str_t XX = 10;

const std::array<MonsterKind, 26> monsters = {{
	/* Name		 CARRY	FLAG    str, exp, lvl, amr, hpt, dmg, maxhp */
	{ "aquator",	0,	CreatureFlag::Mean,	{ XX, 20,   5,   2, NA, "0d0/0d0", NA } },
	{ "bat",	 	0,	CreatureFlag::Flying,	{ XX,  1,   1,   3, NA, "1d2", NA } },
	{ "centaur",	 15,	{},	{ XX, 25,   4,   4, NA, "1d6/1d6", NA } },
	{ "dragon",	 100,	CreatureFlag::Mean,	{ XX,6800, 10,  -1, NA, "1d8/1d8/3d10", NA } },
	{ "emu",	 0,	CreatureFlag::Mean,	{ XX,  2,   1,   7, NA, "1d2", NA } },
		/* until one hits; then every flytrap does fung_hit d1, see flytrap_attacks() */
		/* string with others, since it is written on in the program */
	{ "venus flytrap",0,	CreatureFlag::Mean,	{ XX, 80,   8,   3, NA, "0d0", NA } },
	{ "griffin",	 20,	CreatureFlag::Mean|CreatureFlag::Flying|CreatureFlag::Regen,	{XX,2000, 13, 2,NA, "4d3/3d5/4d3", NA } },
	{ "hobgoblin",	 0,	CreatureFlag::Mean,	{ XX,  3,   1,   5, NA, "1d8", NA } },
	{ "ice monster", 0,	CreatureFlag::Mean,	{ XX,  15,   1,   9, NA, "1d2", NA } },
	{ "jabberwock",  70,	{},	{ XX,4000, 15,   6, NA, "2d12/2d4", NA } },
	{ "kestral",	 0,	CreatureFlag::Mean|CreatureFlag::Flying, { XX,  1,   1,   7, NA, "1d4", NA } },
		/*
		 * The original has ISGREED (0x40) in the CARRY column: leprechauns
		 * carry something 64% of the time and are not greedy. Kept as is.
		 */
	{ "leprechaun",	 0x40,	{},	{ XX, 10,   3,   8, NA, "1d2", NA } },
	{ "medusa",	 40,	CreatureFlag::Mean,	{ XX,200,   8,   2, NA, "3d4/3d4/2d5", NA } },
	{ "nymph",	 100,	{},	{ XX, 37,   3,   9, NA, "0d0", NA } },
	{ "orc",	 15,	CreatureFlag::Greedy,{ XX,  5,   1,   6, NA, "1d8", NA } },
	{ "phantom",	 0,CreatureFlag::Invisible,{ XX,120,   8,   3, NA, "4d4", NA } },
	{ "quagga",	 30,	CreatureFlag::Mean,	{ XX, 32,   3,   2, NA, "1d2/1d2/1d4", NA } },
	{ "rattlesnake", 0,	CreatureFlag::Mean,	{ XX,  9,   2,   3, NA, "1d6", NA } },
	{ "slime",	 	 0,	CreatureFlag::Mean,	{ XX,  1,   2,   8, NA, "1d3", NA } },
	{ "troll",	 50,	CreatureFlag::Regen|CreatureFlag::Mean,{ XX, 120, 6, 4, NA, "1d8/1d8/2d6", NA } },
	{ "ur-vile",	 0,	CreatureFlag::Mean,	{ XX,190,   7,  -2, NA, "1d3/1d3/1d3/4d6", NA } },
	{ "vampire",	 20,	CreatureFlag::Regen|CreatureFlag::Mean,{ XX,350,   8,   1, NA, "1d10", NA } },
	{ "wraith",	 0,	{},	{ XX, 55,   5,   4, NA, "1d6", NA } },
	{ "xeroc",30,	{},	{ XX,100,   7,   7, NA, "3d4", NA } },
	{ "yeti",	 30,	{},	{ XX, 50,   4,   6, NA, "1d6/1d6", NA } },
	{ "zombie",	 0,	CreatureFlag::Mean,	{ XX,  6,   2,   8, NA, "1d8", NA } }
}};

namespace {

int	exp_add(const Creature &tp);

}  // namespace

/*
 * List of monsters in rough order of vorpalness
 */

/*
 * Note:  vorp_mons was not present in the original v1.48 code.  It is used to
 * select a target for the Vorpalize Weapon scroll.
 * Previously, lvl_mons was used, which contains spaces.  When a space
 * character was selected, identifying the player's weapon then indexed the
 * monsters array out-of-bounds, causing a segfault.
 *
 * From disassembling earlier Rogue PC versions, we can deduce that lvl_mons
 * originally had no spaces when the Vorpalize scroll was introduced, so
 * re-introducing this string with no spaces is believed to reproduce the
 * intended behavior.
 */

constexpr std::string_view vorp_mons = "KEBHISORZLCAQNYTWFPUGMXVJD";
constexpr std::string_view lvl_mons =  "K BHISOR LCA NYTWFP GMXVJD";
constexpr std::string_view wand_mons = "KEBHISORZ CAQ YTW PUGM VJ ";

/*
 * randmonster:
 *	Pick a monster to show up.  The lower the level,
 *	the meaner the monster.
 */
char
randmonster(bool wander)
{
	int d;
	std::string_view mons = wander ? wand_mons : lvl_mons;

	do {
		int r10 = rnd(5) + rnd(6);

		d = game().level.depth + (r10 - 5);
		if (d < 1)
			d = rnd(5) + 1;
		if (d > 26)
			d = rnd(5) + 22;
	} while (mons[--d] == ' ');
	return mons[d];
}

/*
 * new_monster:
 *	Pick a new monster and add it to the list
 */
void
new_monster(Creature &tp, unsigned char type, Coord cp)
{
	int lev_add = game().level.depth - world::AMULETLEVEL;
	if (lev_add < 0)
		lev_add = 0;
	game().level.monsters.push_front(tp);
	tp.type = type;
	tp.disguise = type;
	tp.pos = cp;
	tp.under = '@';
	tp.room = world::roomin(cp);
	const MonsterKind &mp = monsters[tp.type-'A'];
	tp.stats.level = mp.m_stats.level + lev_add;
	tp.stats.max_hp = tp.stats.hp = roll(tp.stats.level, 8);
	tp.stats.armor = mp.m_stats.armor - lev_add;
	tp.stats.damage = mp.m_stats.damage;
	tp.stats.str = mp.m_stats.str;
	tp.stats.exp = mp.m_stats.exp + lev_add * 10 + exp_add(tp);
	tp.flags = mp.m_flags;
	tp.its_turn = true;
	tp.pack.clear();
	if (game().player.wears(Ring::AggravateMonster))
		start_run(cp);
	if (type == 'X')
	{
		switch (rnd(game().level.depth > 25 ? 9 : 8))
		{
		case 0: tp.disguise = GOLD; break;
		case 1: tp.disguise = POTION; break;
		case 2: tp.disguise = SCROLL; break;
		case 3: tp.disguise = STAIRS; break;
		case 4: tp.disguise = WEAPON; break;
		case 5: tp.disguise = ARMOR; break;
		case 6: tp.disguise = RING; break;
		case 7: tp.disguise = STICK; break;
		case 8: tp.disguise = AMULET;
		break;
		}
	}
}

/*
 *  f_restor(): restore the initial damage of flytraps
 */
void
f_restor()
{
	game().player.fung_hit = 0;
}

/*
 * flytrap_attacks:
 *	Every venus flytrap's attack: the table's until one hits, then one die
 *	of one side per hit (was the f_damage buffer that all their damage
 *	pointed at)
 */
rogue::Attacks
flytrap_attacks(int hits)
{
	if (hits == 0)
		return monsters['F'-'A'].m_stats.damage;
	return rogue::Attacks(rogue::Dice{hits, 1});
}

namespace {

/*
 * expadd:
 *	Experience to add for this monster's level/hit points
 */
int
exp_add(const Creature &tp)
{
	int mod = tp.stats.level == 1 ? tp.stats.max_hp / 8 : tp.stats.max_hp / 6;
	if (tp.stats.level > 9)
		mod *= 20;
	else if (tp.stats.level > 6)
		mod *= 4;
	return mod;
}

}  // namespace

/*
 * wanderer:
 *	Create a new wandering monster and aim it at the player
 */
void
wanderer()
{
	rogue::Player &player = game().player;

	/*
	 * can we allocate a new monster
	 */
	Maybe<Creature> tp = new_creature();
	if (!tp)
		return;
	int i;
	Coord cp;
	do {
		i = world::rnd_room();
		if (RoomRef::room(i) == player.body.room)
			continue;
		cp = rnd_pos(game().level.rooms[i]);
	} while (!(RoomRef::room(i) != player.body.room && step_ok(game().level.seen_at(cp))));
	new_monster(*tp, randmonster(true), cp);
	start_run(tp->pos);
}

/*
 * wake_monster:
 *	What to do when the hero steps next to a monster
 */
Maybe<Creature>
wake_monster(int y, int x)
{
	rogue::Player &player = game().player;

	Maybe<Creature> tp = game().level.monster_at({x, y});
	if (!tp)
		return tp;
	unsigned char ch = tp->type;
	/*
	 * Every time he sees mean monster, it might start chasing him
	 */
	if (!tp->flags.test(CreatureFlag::Running) && rnd(3) != 0 && tp->flags.test(CreatureFlag::Mean) && !tp->flags.test(CreatureFlag::Held)
		&& !player.wears(Ring::Stealth))
	{
		tp->dest = Hero{};
		tp->flags.set(CreatureFlag::Running);
	}
	if (ch == 'M' && !player.body.flags.test(CreatureFlag::Blind) && !tp->flags.test(CreatureFlag::Found)
		&& !tp->flags.test(CreatureFlag::Cancelled) && tp->flags.test(CreatureFlag::Running))
	{
		std::optional<RoomRef> rp = player.body.room;
		int dst = distance_sq({x, y}, player.body.pos);
		if ((rp && !game().level.room(*rp).r_flags.test(RoomFlag::Dark)) || dst < world::LAMPDIST) {
			tp->flags.set(CreatureFlag::Found);
			if (!rules::save(rules::SaveThrow::Magic)) {
				if (player.body.flags.test(CreatureFlag::Confused))
					rules::lengthen(rules::Event::Unconfuse, rnd(20) + rules::huh_duration());
				else
					rules::fuse(rules::Event::Unconfuse, rnd(20) + rules::huh_duration());
				player.body.flags.set(CreatureFlag::Confused);
				msg("the medusa's gaze has confused you");
			}
		}
	}
	/*
	 * Let greedy ones guard gold
	 */
	if (tp->flags.test(CreatureFlag::Greedy) && !tp->flags.test(CreatureFlag::Running)) {
		tp->flags.set(CreatureFlag::Running);
		if (game().level.room(*player.body.room).r_goldval)
			tp->dest = Gold{*player.body.room};
		else
			tp->dest = Hero{};
	}
	return tp;
}

/*
 * give_pack:
 *	Give a pack to a monster if it deserves one
 */
void
give_pack(Creature &tp)
{
	/*
	 * check if we can allocate a new item
	 */
	if (game().pool.total < MAXITEMS && rnd(100) < monsters[tp.type-'A'].m_carry)
		tp.pack.push_front(*items::new_thing());
}

/*
 * pick_mons:
 *	Choose a sort of monster for the enemy of a vorpally enchanted weapon
 *
 *	Picks from vorp_mons, which has no spaces; see the comment there.
 */
char
pick_mons()
{
	int i = static_cast<int>(vorp_mons.size());

	while (--i >= 0 && rnd(10))
		;
	if (i < 0)
		return 'M';
	return vorp_mons[i];
}

}  // namespace rogue::entities
