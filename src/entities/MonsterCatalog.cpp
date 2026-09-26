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

static const char *vorp_mons = "KEBHISORZLCAQNYTWFPUGMXVJD";
static const char *lvl_mons =  "K BHISOR LCA NYTWFP GMXVJD";
static const char *wand_mons = "KEBHISORZ CAQ YTW PUGM VJ ";

/*
 * randmonster:
 *	Pick a monster to show up.  The lower the level,
 *	the meaner the monster.
 */
char
randmonster(bool wander)
{
	int d;
	const char *mons;

	mons = wander ? wand_mons : lvl_mons;
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
new_monster(Creature *tp, unsigned char type, coord *cp)
{
	struct monster *mp;
	int lev_add;

	if ((lev_add = game().level.depth - AMULETLEVEL) < 0)
		lev_add = 0;
	attach(game().level.monsters, tp);
	tp->t_type = type;
	tp->t_disguise = type;
	bcopy(tp->t_pos,*cp);
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
	tp->t_turn = TRUE;
	tp->t_pack.clear();
	if (ISWEARING(R_AGGR))
		start_run(cp);
	if (type == 'F')
		tp->t_stats.s_dmg = game().player.flytrap_damage;
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
 *  f_restor(): restor initial damage string for flytraps
 */
void
f_restor(void)
{
	struct monster *mp = &monsters['F'-'A'];

	game().player.fung_hit = 0;
	strcpy(game().player.flytrap_damage, mp->m_stats.s_dmg);
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

	/*
	 * can we allocate a new monster
	 */
	if ((tp = new_creature()) == NULL)
		return;
	do {
		i = rnd_room();
		if ((rp = &game().level.rooms[i]) == proom)
			continue;
		rnd_pos(rp, &cp);
	} while (!(rp != proom && step_ok(winat(cp.y, cp.x))));
	new_monster(tp, randmonster(TRUE), &cp);
	start_run(&tp->t_pos);
}

/*
 * wake_monster:
 *	What to do when the hero steps next to a monster
 */
Creature *
wake_monster(int y, int x)
{
	Creature *tp;
	struct room *rp;
	unsigned char ch;
	int dst;

	if ((tp = moat(y, x)) == NULL)
		return tp;
	ch = tp->t_type;
	/*
	 * Every time he sees mean monster, it might start chasing him
	 */
	if (!tp->t_flags.test(ISRUN) && rnd(3) != 0 && tp->t_flags.test(ISMEAN) && !tp->t_flags.test(ISHELD)
		&& !ISWEARING(R_STEALTH))
	{
		tp->t_dest = &hero;
		tp->t_flags.set(ISRUN);
	}
	if (ch == 'M' && !game().player.body.t_flags.test(ISBLIND) && !tp->t_flags.test(ISFOUND)
		&& !tp->t_flags.test(ISCANC) && tp->t_flags.test(ISRUN))
	{
		rp = proom;
		dst = DISTANCE(y, x, hero.y, hero.x);
		if ((rp != NULL && !rp->r_flags.test(RoomFlag::Dark)) || dst < LAMPDIST) {
			tp->t_flags.set(ISFOUND);
			if (!save(VS_MAGIC)) {
				if (game().player.body.t_flags.test(ISHUH))
					lengthen(Event::Unconfuse, rnd(20) + HUHDURATION);
				else
					fuse(Event::Unconfuse, rnd(20) + HUHDURATION);
				game().player.body.t_flags.set(ISHUH);
				msg("the medusa's gaze has confused you");
			}
		}
	}
	/*
	 * Let greedy ones guard gold
	 */
	if (tp->t_flags.test(ISGREED) && !tp->t_flags.test(ISRUN)) {
		tp->t_flags.set(ISRUN);
		if (proom->r_goldval)
			tp->t_dest = &proom->r_gold;
		else
			tp->t_dest = &hero;
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
		attach(tp->t_pack, new_thing());
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
	const char *cp = vorp_mons + strlen(vorp_mons);

	while (--cp >= vorp_mons && rnd(10))
		;
	if (cp < vorp_mons)
		return 'M';
	return *cp;
}


/*
 * moat(x,y)
 *    returns pointer to monster at coordinate
 *	  if no monster there return NULL
 */

Creature *
moat(int my, int mx)
{
	Creature *tp;

	for (tp = game().level.monsters.first(); tp != NULL; tp = game().level.monsters.after(tp))
		if (tp->t_pos.x == mx  && tp->t_pos.y == my)
			return(tp);
	return(NULL);
}

}  // namespace rogue::entities
