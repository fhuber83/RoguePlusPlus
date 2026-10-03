#include "items/effects/Ring.hpp"

#include <optional>
#include <string>

#include "core/Glyphs.hpp"
#include "core/Maybe.hpp"
#include "entities/Item.hpp"
#include "entities/MonsterAI.hpp"
#include "game/Game.hpp"
#include "game/Keyboard.hpp"
#include "game/Messages.hpp"
#include "items/Identification.hpp"
#include "items/Inventory.hpp"
#include "items/Kinds.hpp"
#include "items/effects/Potion.hpp"
#include "items/effects/Weapon.hpp"

namespace rogue::items::effects {

namespace {

std::optional<Hand>	gethand();

/*
 * put_ring_on:
 *	Put a ring on a hand: false if none went on
 */
bool
put_ring_on()
{
	rogue::Player &player = game().player;

	Maybe<Item> obj = get_item("put on", ItemKind::Ring);
	if (!obj)
		return false;
	/*
	 * Make certain that it is somethings that we want to wear
	 */
	if (obj->kind != ItemKind::Ring) {
		msg("you can't put that on your finger");
		return false;
	}

	/*
	 * find out which hand to put it on
	 */
	if (is_current(*obj))
		return false;

	std::optional<Hand> ring;
	if (!player.ring_item(Hand::Left))
		ring = Hand::Left;
	if (!player.ring_item(Hand::Right))
		ring = Hand::Right;
	if (!player.ring_item(Hand::Left) && !player.ring_item(Hand::Right))
		if (!(ring = gethand()))
			return false;
	if (!ring) {
		msg("you already have a ring on each hand");
		return false;
	}
	player.rings[*ring] = game().pool.id_of(obj);

	/*
	 * Calculate the effect it has on the poor guy.
	 */
	switch (obj->which<Ring>()) {
	case Ring::AddStrength:
		player.change_strength(obj->ac);
		break;
	case Ring::SeeInvisible:
		invis_on();
		break;
	case Ring::AggravateMonster:
		entities::aggravate();
		break;
	default:
		break;
	}

	msg("{}wearing {} ({:c})", noterse("you are now "),
		inv_name(*obj, true), pack_char(*obj));
	return true;
}

}  // namespace

/*
 * ring_on:
 *	Put a ring on a hand
 */
void
ring_on()
{
	if (!put_ring_on())
		game().turn.after = false;
}

/*
 * ring_off:
 *	Take off a ring
 */
void
ring_off()
{
	rogue::Player &player = game().player;

	Hand ring;

	if (!player.ring_item(Hand::Left) && !player.ring_item(Hand::Right)) {
		msg("you aren't wearing any rings");
		game().turn.after = false;
		return;
	} else if (!player.ring_item(Hand::Left))
		ring = Hand::Right;
	else if (!player.ring_item(Hand::Right))
		ring = Hand::Left;
	else if (std::optional<Hand> hand = gethand())
		ring = *hand;
	else
		return;
	game().message.end = 0;
	Maybe<Item> obj = player.ring_item(ring);
	if (!obj) {
		msg("not wearing such a ring");
		game().turn.after = false;
		return;
	}
	char packchar = pack_char(*obj);
	if (can_drop(*obj))
		msg("was wearing {}({:c})", inv_name(*obj, true), packchar);
}

namespace {

/*
 * gethand:
 *	Which hand is the hero interested in?
 */
std::optional<Hand>
gethand()
{
	for (;;) {
		msg("left hand or right hand? ");
		int c = readchar();
		if (c == ESCAPE)  {
			game().turn.after = false;
			return std::nullopt;
		}
		game().message.end = 0;
		if (c == 'l' || c == 'L')
			return Hand::Left;
		else if (c == 'r' || c == 'R')
			return Hand::Right;
		msg("please type L or R");
	}
}

}  // namespace

/*
 * ring_num:
 *	Print ring bonuses
 */
std::string
ring_num(const Item &obj)
{
	if (!obj.is(ItemFlag::Known))
		return "";
	switch (obj.which<Ring>()) {
	case Ring::Protection:
	case Ring::AddStrength:
	case Ring::IncreaseDamage:
	case Ring::Dexterity:
		return " " + num(obj.ac, 0, RING);
	default:
		return "";
	}
}

}  // namespace rogue::items::effects
