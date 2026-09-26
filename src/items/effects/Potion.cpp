#include "rogue.h"

namespace rogue::items::effects {

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
	switch (obj->which<Potion>())
	{
	case Potion::Confusion:
		items.p_know[Potion::Confusion] = TRUE;
		if (!player.body.t_flags.test(ISHUH))
			{
			if (player.body.t_flags.test(ISHUH))
				lengthen(Event::Unconfuse, rnd(8)+huh_duration());
			else
				fuse(Event::Unconfuse, rnd(8)+huh_duration());
			player.body.t_flags.set(ISHUH);
			msg("wait, what's going on? Huh? What? Who?");
		}
		break;
	case Potion::Poison:
		{
		constexpr const char *sick = "you feel {} sick.";

		items.p_know[Potion::Poison] = TRUE;
		if (!player.wears(Ring::SustainStrength))
		{
			chg_str(-(rnd(3)+1));
			msg(sick, "very");
		}
		else
			msg(sick, "momentarily");
		}
		break;
	case Potion::Healing:
		items.p_know[Potion::Healing] = TRUE;
		if ((pstats.s_hpt += roll(pstats.s_lvl, 4)) > max_hp)
			pstats.s_hpt = ++max_hp;
		sight();
		msg("you begin to feel better");
		break;
	case Potion::GainStrength:
		items.p_know[Potion::GainStrength] = TRUE;
		chg_str(1);
		msg("you feel stronger. What bulging muscles!");
		break;
	case Potion::MonsterDetection:
		fuse(Event::TurnSeeOff, huh_duration());
		if (game().level.monsters.empty())
			msg("you have a strange feeling{}.",
				noterse(" for a moment"));
		else
		{
			if (turn_see(FALSE))
			{
				items.p_know[Potion::MonsterDetection] = TRUE;
			}
			msg("");
		}
		break;
	case Potion::MagicDetection:
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
					items.p_know[Potion::MagicDetection] = TRUE;
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
						items.p_know[Potion::MagicDetection] = TRUE;
					}
				}
			}
			if (show)
			{
				msg("You sense the presence of magic.");
				break;
			}
		}
		msg("you have a strange feeling for a moment{}.",
				noterse(", then it passes"));
		break;
	case Potion::Paralysis:
		items.p_know[Potion::Paralysis] = TRUE;
		player.no_command = hold_time();
		player.body.t_flags.unset(ISRUN);
		msg("you can't move");
		break;
	case Potion::SeeInvisible:
		if (!player.body.t_flags.test(CANSEE)) {
			fuse(Event::Unsee, see_duration());
			look(FALSE);
			invis_on();
		}
		sight();
		msg("this potion tastes like {} juice", game().options.fruit);
		break;
	case Potion::RaiseLevel:
		items.p_know[Potion::RaiseLevel] = TRUE;
		msg("you suddenly feel much more skillful");
		raise_level();
		break;
	case Potion::ExtraHealing:
		items.p_know[Potion::ExtraHealing] = TRUE;
		if ((pstats.s_hpt += roll(pstats.s_lvl, 8)) > max_hp)
		{
			if (pstats.s_hpt > max_hp + pstats.s_lvl + 1)
				++max_hp;
			pstats.s_hpt = ++max_hp;
		}
		sight();
		msg("you begin to feel much better");
		break;
	case Potion::Haste:
		items.p_know[Potion::Haste] = TRUE;
		if (add_haste(TRUE))
			msg("you feel yourself moving much faster");
		break;
	case Potion::RestoreStrength:
		if (player.wears(Hand::Left, Ring::AddStrength))
			add_str(&pstats.s_str, -player.rings[Hand::Left]->o_ac);
		if (player.wears(Hand::Right, Ring::AddStrength))
			add_str(&pstats.s_str, -player.rings[Hand::Right]->o_ac);
		if (pstats.s_str < player.max_stats.s_str)
			pstats.s_str = player.max_stats.s_str;
		if (player.wears(Hand::Left, Ring::AddStrength))
			add_str(&pstats.s_str, player.rings[Hand::Left]->o_ac);
		if (player.wears(Hand::Right, Ring::AddStrength))
			add_str(&pstats.s_str, player.rings[Hand::Right]->o_ac);
		msg("{}you feel warm all over",
			noterse("hey, this tastes great.  It makes "));
		break;
	case Potion::Blindness:
		items.p_know[Potion::Blindness] = TRUE;
		if (!player.body.t_flags.test(ISBLIND))
		{
			player.body.t_flags.set(ISBLIND);
			fuse(Event::Sight, see_duration());
			look(FALSE);
		}
		msg("a cloak of darkness falls around you");
		break;
	case Potion::ThirstQuenching:
		msg("this potion tastes extremely dull");
		break;
	default:
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
		pack.remove(obj);
		discardit = TRUE;
	}

	call_it(items.p_know[obj->which<Potion>()], &items.p_guess[obj->which<Potion>()]);

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
	if (th->t_flags.test(ISINVIS) && see_monst(th))
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
	unsigned char was_there = ' ';

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
	switch (obj->which<Potion>())
	{
	case Potion::Confusion:
	case Potion::Blindness:
		tp->t_flags.set(ISHUH);
		msg("the {} appears confused", monsters[tp->t_type-'A'].m_name);
		break;
	case Potion::Paralysis:
		tp->t_flags.unset(ISRUN);
		tp->t_flags.set(ISHELD);
		break;
	case Potion::Healing:
	case Potion::ExtraHealing:
		if ((tp->t_stats.s_hpt += rnd(8)) > tp->t_stats.s_maxhp)
		tp->t_stats.s_hpt = ++tp->t_stats.s_maxhp;
		break;
	case Potion::RaiseLevel:
		tp->t_stats.s_hpt += 8;
		tp->t_stats.s_maxhp += 8;
		tp->t_stats.s_lvl++;
		break;
	case Potion::Haste:
		tp->t_flags.set(ISHASTE);
		break;
	default:
		break;
	}
	msg("the flask shatters.");
}

}  // namespace rogue::items::effects
