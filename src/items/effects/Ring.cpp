#include "rogue.h"

namespace rogue::items::effects {

static std::optional<Hand>	gethand(void);

/*
 * ring_on:
 *	Put a ring on a hand
 */
void
ring_on()
{
	Item *obj;
	std::optional<Hand> ring;
	rogue::Player &player = game().player;

	if ((obj = get_item("put on", ItemKind::Ring)) == nullptr)
		goto no_ring;
	/*
	 * Make certain that it is somethings that we want to wear
	 */
	if (obj->o_type != ItemKind::Ring) {
		msg("you can't put that on your finger");
		goto no_ring;
	}

	/*
	 * find out which hand to put it on
	 */
	if (is_current(*obj))
		goto no_ring;

	if (player.ring_item(Hand::Left) == nullptr)
		ring = Hand::Left;
	if (player.ring_item(Hand::Right) == nullptr)
		ring = Hand::Right;
	if (player.ring_item(Hand::Left) == nullptr && player.ring_item(Hand::Right) == nullptr)
		if (!(ring = gethand()))
			goto no_ring;
	if (!ring) {
		msg("you already have a ring on each hand");
		goto no_ring;
	}
	player.rings[*ring] = game().pool.id_of(obj);

	/*
	 * Calculate the effect it has on the poor guy.
	 */
	switch (obj->which<Ring>()) {
	case Ring::AddStrength:
		chg_str(obj->o_ac);
		break;
	case Ring::SeeInvisible:
		invis_on();
		break;
	case Ring::AggravateMonster:
		aggravate();
		break;
	default:
		break;
	}

	msg("{}wearing {} ({:c})", noterse("you are now "),
		inv_name(*obj, true), pack_char(*obj));
	return ;

no_ring:
	game().turn.after = false;
	return;
}

/*
 * ring_off:
 *	Take off a ring
 */
void
ring_off(void)
{
	Hand ring;
	Item *obj;
	char packchar;
	rogue::Player &player = game().player;

	if (player.ring_item(Hand::Left) == nullptr && player.ring_item(Hand::Right) == nullptr) {
		msg("you aren't wearing any rings");
		game().turn.after = false;
		return;
	} else if (player.ring_item(Hand::Left) == nullptr)
		ring = Hand::Right;
	else if (player.ring_item(Hand::Right) == nullptr)
		ring = Hand::Left;
	else if (std::optional<Hand> hand = gethand())
		ring = *hand;
	else
		return;
	game().message.end = 0;
	obj = player.ring_item(ring);
	if (obj == nullptr) {
		msg("not wearing such a ring");
		game().turn.after = false;
		return;
	}
	packchar = pack_char(*obj);
	if (can_drop(*obj))
		msg("was wearing {}({:c})", inv_name(*obj, true), packchar);
}

/*
 * gethand:
 *	Which hand is the hero interested in?
 */
static
std::optional<Hand>
gethand(void)
{
	int c;

	for (;;) {
		msg("left hand or right hand? ");
		if ((c = readchar()) == ESCAPE)  {
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

/*
 * ring_eat:
 *	How much food does this ring use up?
 */
int
ring_eat(Hand hand)
{
	if (game().player.ring_item(hand) == nullptr)
		return 0;
	switch (game().player.ring_item(hand)->which<Ring>()) {
	case Ring::Regeneration:
		return 2;
	case Ring::SustainStrength:
	case Ring::MaintainArmor:
	case Ring::Protection:
	case Ring::AddStrength:
	case Ring::Stealth:
		return 1;
	case Ring::Searching:
		return(rnd(5)==0);
	case Ring::Dexterity:
	case Ring::IncreaseDamage:
		return (rnd(3) == 0);
	case Ring::SlowDigestion:
		return -rnd(2);
	case Ring::SeeInvisible:
		return (rnd(5) == 0);
	default:
		return 0;
	}
}

/*
 * ring_num:
 *	Print ring bonuses
 */
std::string
ring_num(const Item &obj)
{
	if (!obj.o_flags.test(ISKNOW))
		return "";
	switch (obj.which<Ring>()) {
	case Ring::Protection:
	case Ring::AddStrength:
	case Ring::IncreaseDamage:
	case Ring::Dexterity:
		return " " + num(obj.o_ac, 0, RING);
	default:
		return "";
	}
}

}  // namespace rogue::items::effects
