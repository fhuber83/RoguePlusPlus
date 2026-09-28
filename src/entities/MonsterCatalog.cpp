/*
 * File with various monster functions in it
 *
 * monsters.c	1.4 (A.I. Design)	12/14/84
 */

#include "rogue.h"

namespace rogue::entities {

static int	exp_add(Creature *tp);

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
new_monster(Creature *tp, unsigned char type, Coord cp)
{
	const struct monster *mp;
	int lev_add;

	if ((lev_add = game().level.depth - AMULETLEVEL) < 0)
		lev_add = 0;
	game().level.monsters.push_front(tp);
	tp->t_type = type;
	tp->t_disguise = type;
	tp->t_pos = cp;
	tp->t_oldch = '@';
	tp->t_room = roomin(cp);
	mp = &monsters[tp->t_type-'A'];
	tp->t_stats.s_lvl = mp->m_stats.s_lvl + lev_add;
	tp->t_stats.s_maxhp = tp->t_stats.s_hpt = roll(tp->t_stats.s_lvl, 8);
	tp->t_stats.s_arm = mp->m_stats.s_arm - lev_add;
	tp->t_stats.s_dmg = mp->m_stats.s_dmg;
	tp->t_stats.s_str = mp->m_stats.s_str;
	tp->t_stats.s_exp = mp->m_stats.s_exp + lev_add * 10 + exp_add(tp);
	tp->t_flags = mp->m_flags;
	tp->t_turn = true;
	tp->t_pack.clear();
	if (game().player.wears(Ring::AggravateMonster))
		start_run(cp);
	if (type == 'X')
	{
		switch (rnd(game().level.depth > 25 ? 9 : 8))
		{
		case 0: tp->t_disguise = GOLD; break;
		case 1: tp->t_disguise = POTION; break;
		case 2: tp->t_disguise = SCROLL; break;
		case 3: tp->t_disguise = STAIRS; break;
		case 4: tp->t_disguise = WEAPON; break;
		case 5: tp->t_disguise = ARMOR; break;
		case 6: tp->t_disguise = RING; break;
		case 7: tp->t_disguise = STICK; break;
		case 8: tp->t_disguise = AMULET;
		break;
		}
	}
}

/*
 *  f_restor(): restore the initial damage of flytraps
 */
void
f_restor(void)
{
	game().player.fung_hit = 0;
}

/*
 * flytrap_attacks:
 *	Every venus flytrap's attack: the table's until one hits, then one die
 *	of one side per hit (was the f_damage buffer that all their s_dmg
 *	pointed at)
 */
rogue::Attacks
flytrap_attacks(int hits)
{
	if (hits == 0)
		return monsters['F'-'A'].m_stats.s_dmg;
	return rogue::Attacks(rogue::Dice{hits, 1});
}

/*
 * expadd:
 *	Experience to add for this monster's level/hit points
 */
static
int
exp_add(Creature *tp)
{
	int mod;

	if (tp->t_stats.s_lvl == 1)
		mod = tp->t_stats.s_maxhp / 8;
	else
		mod = tp->t_stats.s_maxhp / 6;
	if (tp->t_stats.s_lvl > 9)
		mod *= 20;
	else if (tp->t_stats.s_lvl > 6)
		mod *= 4;
	return mod;
}

/*
 * wanderer:
 *	Create a new wandering monster and aim it at the player
 */
void
wanderer(void)
{
	int i;
	struct room *rp;
	Creature *tp;
	coord cp;
	rogue::Player &player = game().player;

	/*
	 * can we allocate a new monster
	 */
	if ((tp = new_creature()) == nullptr)
		return;
	do {
		i = rnd_room();
		rp = &game().level.rooms[i];
		if (RoomRef::room(i) == player.body.t_room)
			continue;
		cp = rnd_pos(rp);
	} while (!(RoomRef::room(i) != player.body.t_room && step_ok(winat(cp.y, cp.x))));
	new_monster(tp, randmonster(true), cp);
	start_run(tp->t_pos);
}

/*
 * wake_monster:
 *	What to do when the hero steps next to a monster
 */
Creature *
wake_monster(int y, int x)
{
	Creature *tp;
	std::optional<RoomRef> rp;
	unsigned char ch;
	int dst;
	rogue::Player &player = game().player;

	if ((tp = moat(y, x)) == nullptr)
		return tp;
	ch = tp->t_type;
	/*
	 * Every time he sees mean monster, it might start chasing him
	 */
	if (!tp->t_flags.test(ISRUN) && rnd(3) != 0 && tp->t_flags.test(ISMEAN) && !tp->t_flags.test(ISHELD)
		&& !player.wears(Ring::Stealth))
	{
		tp->t_dest = &player.body.t_pos;
		tp->t_flags.set(ISRUN);
	}
	if (ch == 'M' && !player.body.t_flags.test(ISBLIND) && !tp->t_flags.test(ISFOUND)
		&& !tp->t_flags.test(ISCANC) && tp->t_flags.test(ISRUN))
	{
		rp = player.body.t_room;
		dst = DISTANCE(y, x, player.body.t_pos.y, player.body.t_pos.x);
		if ((rp && !game().level.room(*rp).r_flags.test(RoomFlag::Dark)) || dst < LAMPDIST) {
			tp->t_flags.set(ISFOUND);
			if (!save(SaveThrow::Magic)) {
				if (player.body.t_flags.test(ISHUH))
					lengthen(Event::Unconfuse, rnd(20) + huh_duration());
				else
					fuse(Event::Unconfuse, rnd(20) + huh_duration());
				player.body.t_flags.set(ISHUH);
				msg("the medusa's gaze has confused you");
			}
		}
	}
	/*
	 * Let greedy ones guard gold
	 */
	if (tp->t_flags.test(ISGREED) && !tp->t_flags.test(ISRUN)) {
		tp->t_flags.set(ISRUN);
		if (struct room &here = game().level.room(*player.body.t_room); here.r_goldval)
			tp->t_dest = &here.r_gold;
		else
			tp->t_dest = &player.body.t_pos;
	}
	return tp;
}

/*
 * give_pack:
 *	Give a pack to a monster if it deserves one
 */
void
give_pack(Creature *tp)
{
	/*
	 * check if we can allocate a new item
	 */
	if (game().pool.total < MAXITEMS && rnd(100) < monsters[tp->t_type-'A'].m_carry)
		tp->t_pack.push_front(new_thing());
}

/*
 * pick_mons:
 *	Choose a sort of monster for the enemy of a vorpally enchanted weapon
 *
 *	Picks from vorp_mons, which has no spaces; see the comment there.
 */
char
pick_mons(void)
{
	int i = static_cast<int>(vorp_mons.size());

	while (--i >= 0 && rnd(10))
		;
	if (i < 0)
		return 'M';
	return vorp_mons[i];
}


/*
 * moat(x,y)
 *    returns pointer to monster at coordinate
 *	  if no monster there return null
 */

Creature *
moat(int my, int mx)
{
	Creature *tp;

	for (tp = game().level.monsters.first(); tp != nullptr; tp = game().level.monsters.after(tp))
		if (tp->t_pos.x == mx  && tp->t_pos.y == my)
			return(tp);
	return(nullptr);
}

}  // namespace rogue::entities
