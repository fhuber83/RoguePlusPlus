#include "rogue.h"

namespace rogue::items::effects {

namespace {

/*
 * add_haste:
 *	Add a haste to the player
 */
bool
add_haste(bool potion)
{
	rogue::Player &player = game().player;
	if (player.body.t_flags.test(ISHASTE))
	{
		player.no_command += rnd(8);
		player.body.t_flags.unset(ISRUN);
		extinguish(Event::NoHaste);
		player.body.t_flags.unset(ISHASTE);
		msg("you faint from exhaustion");
		return false;
	}
	else
	{
		player.body.t_flags.set(ISHASTE);
		if (potion)
			fuse(Event::NoHaste, rnd(4)+10);
		return true;
	}
}

/*
 * goodch:
 *	Decide how good an object is and return the correct character for
 * printing.
 */
char
goodch(const Item &obj)
{
	char ch = MAGIC;

	if (obj.o_flags.test(ISCURSED))
		ch = BMAGIC;
	switch (obj.o_type) {
	case ItemKind::Armor:
		if (obj.o_ac > a_class[obj.which<ArmorType>()])
			ch = BMAGIC;
		break;
	case ItemKind::Weapon:
		if (obj.o_hplus < 0 || obj.o_dplus < 0)
			ch = BMAGIC;
		break;
	case ItemKind::Scroll:
		switch (obj.which<Scroll>()) {
		case Scroll::Sleep:
		case Scroll::CreateMonster:
		case Scroll::AggravateMonsters:
			ch = BMAGIC;
			break;
		default:
			break;
		}
		break;
	case ItemKind::Potion:
		switch (obj.which<Potion>()) {
		case Potion::Confusion:
		case Potion::Paralysis:
		case Potion::Poison:
		case Potion::Blindness:
			ch = BMAGIC;
			break;
		default:
			break;
		}
		break;
	case ItemKind::Stick:
		switch (obj.which<Stick>()) {
		case Stick::HasteMonster:
		case Stick::TeleportTo:
			ch = BMAGIC;
			break;
		default:
			break;
		}
		break;
	case ItemKind::Ring:
		switch (obj.which<Ring>()) {
		case Ring::Protection:
		case Ring::AddStrength:
		case Ring::IncreaseDamage:
		case Ring::Dexterity:
			if (obj.o_ac < 0)
				ch = BMAGIC;
			break;
		case Ring::AggravateMonster:
		case Ring::Teleportation:
			ch = BMAGIC;
			break;
		default:
			break;
		}
		break;
	default:	// the other kinds of item: nothing
		break;
	}
	return ch;
}

}  // namespace

/*
 * quaff:
 *	Quaff a potion from the pack
 */
void
quaff()
{
	Maybe<Item> obj;
	Maybe<Creature> th;
	bool discardit = false;
	rogue::Player &player = game().player;
	rogue::Items &items = game().items;

	if (!(obj = get_item("quaff", ItemKind::Potion)))
		return;
	/*
	 * Make certain that it is somethings that we want to drink
	 */
	if (obj->o_type != ItemKind::Potion)
	{
		msg("yuk! Why would you want to drink that?");
		return;
	}
	if (obj == player.weapon_item())
		player.weapon = std::nullopt;

	/*
	 * Calculate the effect it has on the poor guy.
	 */
	switch (obj->which<Potion>())
	{
	case Potion::Confusion:
		items.p_know[Potion::Confusion] = true;
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
		constexpr std::string_view sick = "you feel {} sick.";

		items.p_know[Potion::Poison] = true;
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
		items.p_know[Potion::Healing] = true;
		if ((player.body.t_stats.s_hpt += roll(player.body.t_stats.s_lvl, 4)) > player.body.t_stats.s_maxhp)
			player.body.t_stats.s_hpt = ++player.body.t_stats.s_maxhp;
		sight();
		msg("you begin to feel better");
		break;
	case Potion::GainStrength:
		items.p_know[Potion::GainStrength] = true;
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
			if (turn_see(false))
			{
				items.p_know[Potion::MonsterDetection] = true;
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
			Maybe<Item> tp;
			bool show;

			show = false;
			for (tp = game().level.objects.first(); tp; tp = game().level.objects.after(*tp))
			{
				if (is_magic(*tp))
				{
					show = true;
					display().draw_tile(tp->o_pos, goodch(*tp));
					items.p_know[Potion::MagicDetection] = true;
				}
			}
			for (th = game().level.monsters.first(); th; th = game().level.monsters.after(*th))
			{
				for (tp = th->t_pack.first(); tp; tp = th->t_pack.after(*tp))
				{
					if (is_magic(*tp))
					{
						show = true;
						display().draw_tile(th->t_pos, MAGIC);
						items.p_know[Potion::MagicDetection] = true;
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
		items.p_know[Potion::Paralysis] = true;
		player.no_command = hold_time();
		player.body.t_flags.unset(ISRUN);
		msg("you can't move");
		break;
	case Potion::SeeInvisible:
		if (!player.body.t_flags.test(CANSEE)) {
			fuse(Event::Unsee, see_duration());
			look(false);
			invis_on();
		}
		sight();
		msg("this potion tastes like {} juice", game().options.fruit);
		break;
	case Potion::RaiseLevel:
		items.p_know[Potion::RaiseLevel] = true;
		msg("you suddenly feel much more skillful");
		raise_level();
		break;
	case Potion::ExtraHealing:
		items.p_know[Potion::ExtraHealing] = true;
		if ((player.body.t_stats.s_hpt += roll(player.body.t_stats.s_lvl, 8)) > player.body.t_stats.s_maxhp)
		{
			if (player.body.t_stats.s_hpt > player.body.t_stats.s_maxhp + player.body.t_stats.s_lvl + 1)
				++player.body.t_stats.s_maxhp;
			player.body.t_stats.s_hpt = ++player.body.t_stats.s_maxhp;
		}
		sight();
		msg("you begin to feel much better");
		break;
	case Potion::Haste:
		items.p_know[Potion::Haste] = true;
		if (add_haste(true))
			msg("you feel yourself moving much faster");
		break;
	case Potion::RestoreStrength:
		if (player.wears(Hand::Left, Ring::AddStrength))
			add_str(player.body.t_stats.s_str, -player.ring_item(Hand::Left)->o_ac);
		if (player.wears(Hand::Right, Ring::AddStrength))
			add_str(player.body.t_stats.s_str, -player.ring_item(Hand::Right)->o_ac);
		if (player.body.t_stats.s_str < player.max_stats.s_str)
			player.body.t_stats.s_str = player.max_stats.s_str;
		if (player.wears(Hand::Left, Ring::AddStrength))
			add_str(player.body.t_stats.s_str, player.ring_item(Hand::Left)->o_ac);
		if (player.wears(Hand::Right, Ring::AddStrength))
			add_str(player.body.t_stats.s_str, player.ring_item(Hand::Right)->o_ac);
		msg("{}you feel warm all over",
			noterse("hey, this tastes great.  It makes "));
		break;
	case Potion::Blindness:
		items.p_know[Potion::Blindness] = true;
		if (!player.body.t_flags.test(ISBLIND))
		{
			player.body.t_flags.set(ISBLIND);
			fuse(Event::Sight, see_duration());
			look(false);
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
		player.body.t_pack.remove(*obj);
		discardit = true;
	}

	call_it(items.p_know[obj->which<Potion>()], items.p_guess[obj->which<Potion>()]);

	if (discardit)
		discard(*obj);
}

/*
 * invis_on:
 *	Turn on the ability to see invisible
 */
void
invis_on()
{
	Maybe<Creature> th;

	game().player.body.t_flags.set(CANSEE);
	for (th = game().level.monsters.first(); th; th = game().level.monsters.after(*th))
	if (th->t_flags.test(ISINVIS) && see_monst(*th))
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
	Maybe<Creature> mp;
	bool can_see, add_new;
	unsigned char was_there = ' ';

	add_new = false;
	for (mp = game().level.monsters.first(); mp; mp = game().level.monsters.after(*mp)) {
		can_see = (see_monst(*mp) || (was_there = display().tile_at(mp->t_pos)) == mp->t_type);
		if (turn_off) {
			if (!see_monst(*mp) && mp->t_oldch != '@')
				display().draw_tile(mp->t_pos, mp->t_oldch);
		} else {
			if (!can_see) {
				mp->t_oldch = was_there;
				add_new = true;
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
th_effect(const Item &obj, Creature &tp)
{
	switch (obj.which<Potion>())
	{
	case Potion::Confusion:
	case Potion::Blindness:
		tp.t_flags.set(ISHUH);
		msg("the {} appears confused", monsters[tp.t_type-'A'].m_name);
		break;
	case Potion::Paralysis:
		tp.t_flags.unset(ISRUN);
		tp.t_flags.set(ISHELD);
		break;
	case Potion::Healing:
	case Potion::ExtraHealing:
		if ((tp.t_stats.s_hpt += rnd(8)) > tp.t_stats.s_maxhp)
		tp.t_stats.s_hpt = ++tp.t_stats.s_maxhp;
		break;
	case Potion::RaiseLevel:
		tp.t_stats.s_hpt += 8;
		tp.t_stats.s_maxhp += 8;
		tp.t_stats.s_lvl++;
		break;
	case Potion::Haste:
		tp.t_flags.set(ISHASTE);
		break;
	default:
		break;
	}
	msg("the flask shatters.");
}

}  // namespace rogue::items::effects
