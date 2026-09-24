#include "rogue.h"

namespace rogue::items::effects {

//@ turn_see() wrapper to use as a fuse
static
void
turn_see_off(void)
{
	turn_see(TRUE);
}

/*
 * quaff:
 *	Quaff a potion from the pack
 */
void
quaff(void)
{
	Item *obj;
	Creature *th;
	bool discardit = FALSE;
	rogue::Player &player = game().player;
	rogue::Items &items = game().items;

	if ((obj = get_item("quaff", ItemKind::Potion)) == NULL)
		return;
	/*
	 * Make certain that it is somethings that we want to drink
	 */
	if (obj->o_type != ItemKind::Potion)
	{
		msg("yuk! Why would you want to drink that?");
		return;
	}
	if (obj == player.weapon)
		player.weapon = NULL;

	/*
	 * Calculate the effect it has on the poor guy.
	 */
	switch (obj->o_which)
	{
	when P_CONFUSE:
		items.p_know[P_CONFUSE] = TRUE;
		if (!on(player.body, ISHUH))
			{
			if (on(player.body, ISHUH))
				lengthen(unconfuse, rnd(8)+HUHDURATION);
			else
				fuse(unconfuse, rnd(8)+HUHDURATION);
			player.body.t_flags.set(ISHUH);
			msg("wait, what's going on? Huh? What? Who?");
		}
	when P_POISON:
		{
		const char *sick = "you feel %s sick.";

		items.p_know[P_POISON] = TRUE;
		if (!ISWEARING(R_SUSTSTR))
		{
			chg_str(-(rnd(3)+1));
			msg(sick, "very");
		}
		else
			msg(sick, "momentarily");
		}
	when P_HEALING:
		items.p_know[P_HEALING] = TRUE;
		if ((pstats.s_hpt += roll(pstats.s_lvl, 4)) > max_hp)
			pstats.s_hpt = ++max_hp;
		sight();
		msg("you begin to feel better");
	when P_STRENGTH:
		items.p_know[P_STRENGTH] = TRUE;
		chg_str(1);
		msg("you feel stronger. What bulging muscles!");
	when P_MFIND:
		fuse(turn_see_off, HUHDURATION);
		if (game().level.monsters.empty())
			msg("you have a strange feeling%s.",
				noterse(" for a moment"));
		else
		{
			if (turn_see(FALSE))
			{
				items.p_know[P_MFIND] = TRUE;
			}
			msg("");
		}
	  when P_TFIND:
		/*
		 * Potion of magic detection.  Find everything interesting on
		 * the level and show him where they are.  Also give hints as
		 * to whether he would want to use the object.
		 */
		if (!game().level.objects.empty())
		{
			Item *tp;
			bool show;

			show = FALSE;
			for (tp = game().level.objects.first(); tp != NULL; tp = game().level.objects.after(tp))
			{
				if (is_magic(tp))
				{
					show = TRUE;
					display().draw_tile(tp->o_pos, goodch(tp));
					items.p_know[P_TFIND] = TRUE;
				}
			}
			for (th = game().level.monsters.first(); th != NULL; th = game().level.monsters.after(th))
			{
				for (tp = th->t_pack.first(); tp != NULL; tp = th->t_pack.after(tp))
				{
					if (is_magic(tp))
					{
						show = TRUE;
						display().draw_tile(th->t_pos, MAGIC);
						items.p_know[P_TFIND] = TRUE;
					}
				}
			}
			if (show)
			{
				msg("You sense the presence of magic.");
				break;
			}
		}
		msg("you have a strange feeling for a moment%s.",
				noterse(", then it passes"));
	when P_PARALYZE:
		items.p_know[P_PARALYZE] = TRUE;
		player.no_command = HOLDTIME;
		player.body.t_flags.unset(ISRUN);
		msg("you can't move");
	when P_SEEINVIS:
		if (!on(player.body, CANSEE)) {
			fuse(unsee, SEEDURATION);
			look(FALSE);
			invis_on();
		}
		sight();
		msg("this potion tastes like %s juice", game().options.fruit);
	when P_RAISE:
		items.p_know[P_RAISE] = TRUE;
		msg("you suddenly feel much more skillful");
		raise_level();
	when P_XHEAL:
		items.p_know[P_XHEAL] = TRUE;
		if ((pstats.s_hpt += roll(pstats.s_lvl, 8)) > max_hp)
		{
			if (pstats.s_hpt > max_hp + pstats.s_lvl + 1)
				++max_hp;
			pstats.s_hpt = ++max_hp;
		}
		sight();
		msg("you begin to feel much better");
	when P_HASTE:
		items.p_know[P_HASTE] = TRUE;
		if (add_haste(TRUE))
			msg("you feel yourself moving much faster");
	when P_RESTORE:
		if (ISRING(LEFT, R_ADDSTR))
			add_str(&pstats.s_str, -player.rings[LEFT]->o_ac);
		if (ISRING(RIGHT, R_ADDSTR))
			add_str(&pstats.s_str, -player.rings[RIGHT]->o_ac);
		if (pstats.s_str < player.max_stats.s_str)
			pstats.s_str = player.max_stats.s_str;
		if (ISRING(LEFT, R_ADDSTR))
			add_str(&pstats.s_str, player.rings[LEFT]->o_ac);
		if (ISRING(RIGHT, R_ADDSTR))
			add_str(&pstats.s_str, player.rings[RIGHT]->o_ac);
		msg("%syou feel warm all over",
			noterse("hey, this tastes great.  It makes "));
	when P_BLIND:
		items.p_know[P_BLIND] = TRUE;
		if (!on(player.body, ISBLIND))
		{
			player.body.t_flags.set(ISBLIND);
			fuse(sight, SEEDURATION);
			look(FALSE);
		}
		msg("a cloak of darkness falls around you");
	when P_NOP:
		msg("this potion tastes extremely dull");
	otherwise:
		msg("what an odd tasting potion!");
		return;
	}
	status();
	/*
	 * Throw the item away
	 */
	player.in_pack--;
	if (obj->o_count > 1)
		obj->o_count--;
	else
	{
		detach(pack, obj);
		discardit = TRUE;
	}

	call_it(items.p_know[obj->o_which], &items.p_guess[obj->o_which]);

	if (discardit)
		discard(obj);
}

/*
 * invis_on:
 *	Turn on the ability to see invisible
 */
void
invis_on(void)
{
	Creature *th;

	game().player.body.t_flags.set(CANSEE);
	for (th = game().level.monsters.first(); th != NULL; th = game().level.monsters.after(th))
	if (on(*th, ISINVIS) && see_monst(th))
	{
		display().draw_tile(th->t_pos, th->t_disguise);
	}
}

/*
 * turn_see:
 *	Put on or off seeing monsters on this level
 */
bool
turn_see(bool turn_off)
{
	Creature *mp;
	bool can_see, add_new;
	byte was_there = ' ';

	add_new = FALSE;
	for (mp = game().level.monsters.first(); mp != NULL; mp = game().level.monsters.after(mp)) {
		can_see = (see_monst(mp) || (was_there = display().tile_at(mp->t_pos)) == mp->t_type);
		if (turn_off) {
			if (!see_monst(mp) && mp->t_oldch != '@')
				display().draw_tile(mp->t_pos, mp->t_oldch);
		} else {
			if (!can_see) {
				mp->t_oldch = was_there;
				add_new = TRUE;
			}
			display().draw_tile(mp->t_pos, mp->t_type,
					can_see ? TileStyle::Normal : TileStyle::Inverse);
		}
	}
	game().player.body.t_flags.set(SEEMONST);
	if (turn_off)
		game().player.body.t_flags.unset(SEEMONST);
	return add_new;
}

/*
 * th_effect:
 *	Compute the effect of this potion hitting a monster.
 */
void
th_effect(Item *obj, Creature *tp)
{
	switch (obj->o_which)
	{
	when P_CONFUSE:
	case P_BLIND:
		tp->t_flags.set(ISHUH);
		msg("the %s appears confused", monsters[tp->t_type-'A'].m_name);
	when P_PARALYZE:
		tp->t_flags.unset(ISRUN);
		tp->t_flags.set(ISHELD);
	when P_HEALING:
	case P_XHEAL:
		if ((tp->t_stats.s_hpt += rnd(8)) > tp->t_stats.s_maxhp)
		tp->t_stats.s_hpt = ++tp->t_stats.s_maxhp;
	when P_RAISE:
		tp->t_stats.s_hpt += 8;
		tp->t_stats.s_maxhp += 8;
		tp->t_stats.s_lvl++;
	when P_HASTE:
		tp->t_flags.set(ISHASTE);
		break;
	}
	msg("the flask shatters.");
}

}  // namespace rogue::items::effects
