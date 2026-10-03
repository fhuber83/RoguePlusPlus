#include "items/effects/Potion.hpp"

#include <optional>
#include <string_view>

#include "core/Glyphs.hpp"
#include "core/Maybe.hpp"
#include "entities/Creature.hpp"
#include "entities/Item.hpp"
#include "entities/MonsterAI.hpp"
#include "entities/MonsterCatalog.hpp"
#include "game/Game.hpp"
#include "game/Messages.hpp"
#include "game/Pool.hpp"
#include "game/StatusLine.hpp"
#include "items/Identification.hpp"
#include "items/Inventory.hpp"
#include "items/ItemCatalog.hpp"
#include "items/Kinds.hpp"
#include "rules/Combat.hpp"
#include "rules/Conditions.hpp"
#include "rules/Durations.hpp"
#include "rules/Scheduler.hpp"
#include "rules/Strength.hpp"
#include "ui/Display.hpp"
#include "world/Look.hpp"

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
	if (player.body.is(CreatureFlag::Hasted))
	{
		player.no_command += rnd(8);
		player.body.flags.unset(CreatureFlag::Running);
		rules::extinguish(rules::Event::NoHaste);
		player.body.flags.unset(CreatureFlag::Hasted);
		msg("you faint from exhaustion");
		return false;
	}
	else
	{
		player.body.flags.set(CreatureFlag::Hasted);
		if (potion)
			rules::fuse(rules::Event::NoHaste, rnd(4)+10);
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

	if (obj.is(ItemFlag::Cursed))
		ch = BMAGIC;
	switch (obj.kind) {
	case ItemKind::Armor:
		if (obj.ac > a_class[obj.which<ArmorType>()])
			ch = BMAGIC;
		break;
	case ItemKind::Weapon:
		if (obj.hit_plus < 0 || obj.damage_plus < 0)
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
			if (obj.ac < 0)
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
	bool discardit = false;
	rogue::Player &player = game().player;
	rogue::Items &items = game().items;

	Maybe<Item> obj = get_item("quaff", ItemKind::Potion);
	if (!obj)
		return;
	/*
	 * Make certain that it is somethings that we want to drink
	 */
	if (obj->kind != ItemKind::Potion)
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
		if (!player.body.is(CreatureFlag::Confused))
			{
			if (player.body.is(CreatureFlag::Confused))
				rules::lengthen(rules::Event::Unconfuse, rnd(8)+rules::huh_duration());
			else
				rules::fuse(rules::Event::Unconfuse, rnd(8)+rules::huh_duration());
			player.body.flags.set(CreatureFlag::Confused);
			msg("wait, what's going on? Huh? What? Who?");
		}
		break;
	case Potion::Poison:
		{
		constexpr std::string_view sick = "you feel {} sick.";

		items.p_know[Potion::Poison] = true;
		if (!player.wears(Ring::SustainStrength))
		{
			rules::chg_str(-(rnd(3)+1));
			msg(sick, "very");
		}
		else
			msg(sick, "momentarily");
		}
		break;
	case Potion::Healing:
		items.p_know[Potion::Healing] = true;
		if ((player.body.stats.hp += roll(player.body.stats.level, 4)) > player.body.stats.max_hp)
			player.body.stats.hp = ++player.body.stats.max_hp;
		rules::sight();
		msg("you begin to feel better");
		break;
	case Potion::GainStrength:
		items.p_know[Potion::GainStrength] = true;
		rules::chg_str(1);
		msg("you feel stronger. What bulging muscles!");
		break;
	case Potion::MonsterDetection:
		rules::fuse(rules::Event::TurnSeeOff, rules::huh_duration());
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
			bool show = false;
			for (Item &tp : game().level.objects)
			{
				if (tp.is_magic())
				{
					show = true;
					ui::display().draw_tile(tp.pos, goodch(tp));
					items.p_know[Potion::MagicDetection] = true;
				}
			}
			for (Creature &th : game().level.monsters)
			{
				for (Item &tp : th.pack)
				{
					if (tp.is_magic())
					{
						show = true;
						ui::display().draw_tile(th.pos, MAGIC);
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
		player.no_command = rules::hold_time();
		player.body.flags.unset(CreatureFlag::Running);
		msg("you can't move");
		break;
	case Potion::SeeInvisible:
		if (!player.body.is(CreatureFlag::SeeInvisible)) {
			fuse(rules::Event::Unsee, rules::see_duration());
			world::look(false);
			invis_on();
		}
		rules::sight();
		msg("this potion tastes like {} juice", game().options.fruit);
		break;
	case Potion::RaiseLevel:
		items.p_know[Potion::RaiseLevel] = true;
		msg("you suddenly feel much more skillful");
		rules::raise_level();
		break;
	case Potion::ExtraHealing:
		items.p_know[Potion::ExtraHealing] = true;
		if ((player.body.stats.hp += roll(player.body.stats.level, 8)) > player.body.stats.max_hp)
		{
			if (player.body.stats.hp > player.body.stats.max_hp + player.body.stats.level + 1)
				++player.body.stats.max_hp;
			player.body.stats.hp = ++player.body.stats.max_hp;
		}
		rules::sight();
		msg("you begin to feel much better");
		break;
	case Potion::Haste:
		items.p_know[Potion::Haste] = true;
		if (add_haste(true))
			msg("you feel yourself moving much faster");
		break;
	case Potion::RestoreStrength:
		if (player.wears(Hand::Left, Ring::AddStrength))
			rules::add_str(player.body.stats.str, -player.ring_item(Hand::Left)->ac);
		if (player.wears(Hand::Right, Ring::AddStrength))
			rules::add_str(player.body.stats.str, -player.ring_item(Hand::Right)->ac);
		if (player.body.stats.str < player.max_stats.str)
			player.body.stats.str = player.max_stats.str;
		if (player.wears(Hand::Left, Ring::AddStrength))
			rules::add_str(player.body.stats.str, player.ring_item(Hand::Left)->ac);
		if (player.wears(Hand::Right, Ring::AddStrength))
			rules::add_str(player.body.stats.str, player.ring_item(Hand::Right)->ac);
		msg("{}you feel warm all over",
			noterse("hey, this tastes great.  It makes "));
		break;
	case Potion::Blindness:
		items.p_know[Potion::Blindness] = true;
		if (!player.body.is(CreatureFlag::Blind))
		{
			player.body.flags.set(CreatureFlag::Blind);
			fuse(rules::Event::Sight, rules::see_duration());
			world::look(false);
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
	if (obj->count > 1)
		obj->count--;
	else
	{
		player.body.pack.remove(*obj);
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
	game().player.body.flags.set(CreatureFlag::SeeInvisible);
	for (Creature &th : game().level.monsters)
	if (th.is(CreatureFlag::Invisible) && entities::see_monst(th))
	{
		ui::display().draw_tile(th.pos, th.disguise);
	}
}

/*
 * turn_see:
 *	Put on or off seeing monsters on this level
 */
bool
turn_see(bool turn_off)
{
	unsigned char was_there = ' ';	/* kept from one monster to the next */
	bool add_new = false;
	for (Creature &mp : game().level.monsters) {
		bool can_see = (entities::see_monst(mp) || (was_there = ui::display().tile_at(mp.pos)) == mp.type);
		if (turn_off) {
			if (!entities::see_monst(mp) && mp.under != '@')
				ui::display().draw_tile(mp.pos, mp.under);
		} else {
			if (!can_see) {
				mp.under = was_there;
				add_new = true;
			}
			ui::display().draw_tile(mp.pos, mp.type,
					can_see ? ui::TileStyle::Normal : ui::TileStyle::Inverse);
		}
	}
	game().player.body.flags.set(CreatureFlag::SeeMonst);
	if (turn_off)
		game().player.body.flags.unset(CreatureFlag::SeeMonst);
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
		tp.flags.set(CreatureFlag::Confused);
		msg("the {} appears confused", entities::monsters[tp.type-'A'].name);
		break;
	case Potion::Paralysis:
		tp.flags.unset(CreatureFlag::Running);
		tp.flags.set(CreatureFlag::Held);
		break;
	case Potion::Healing:
	case Potion::ExtraHealing:
		if ((tp.stats.hp += rnd(8)) > tp.stats.max_hp)
		tp.stats.hp = ++tp.stats.max_hp;
		break;
	case Potion::RaiseLevel:
		tp.stats.hp += 8;
		tp.stats.max_hp += 8;
		tp.stats.level++;
		break;
	case Potion::Haste:
		tp.flags.set(CreatureFlag::Hasted);
		break;
	default:
		break;
	}
	msg("the flask shatters.");
}

}  // namespace rogue::items::effects
