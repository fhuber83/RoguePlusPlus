/*
 * Eating.
 *
 * eat() comes from misc.c, and the stomach daemon from daemons.c.
 *
 * misc.c	1.4		(A.I. Design)	12/14/84
 * @(#)daemons.c	5.1 (Berkeley) 5/11/82
 */

#include "rogue.h"

namespace rogue::rules {

namespace {

constexpr int MORETIME = 150;		/* food left when he gets weak; twice that, hungry */
constexpr int STOMACHSIZE = 2000;	/* food a stomach holds */
constexpr int STARVETIME = 850;		/* how far below empty he starves */

}  // namespace

/*
 * eat:
 *	She wants to eat something, so let her try
 */
void
eat()
{
	Maybe<Item> obj;
	Food which;
	rogue::Player &player = game().player;

	if (!(obj = get_item("eat", ItemKind::Food)))
		return;
	if (obj->o_type != ItemKind::Food)
	{
		msg("ugh, you would get ill if you ate that");
		return;
	}
	player.in_pack--;
	/*
	 * What it is, and whether it was wielded, are checked before the last
	 * one is discarded. Both were after discard(), reading a freed item.
	 */
	which = obj->which<Food>();
	if (obj == player.weapon_item())
		player.weapon = std::nullopt;
	if (--obj->o_count < 1)
	{
		player.body.t_pack.remove(*obj);
		discard(*obj);
	}
	if (player.food_left < 0)
		player.food_left = 0;
	if (player.food_left > (STOMACHSIZE - 20))
		player.no_command += 2 + rnd(5);
	if ((player.food_left += hunger_time() - 200 + rnd(400)) > STOMACHSIZE)
		player.food_left = STOMACHSIZE;
	player.hungry_state = 0;
	if (which == Food::Fruit)
		msg("my, that was a yummy {}", game().options.fruit);
	else
		if (rnd(100) > 70)
		{
			player.body.t_stats.s_exp++;
			msg("yuk, this food tastes awful");
			check_level();
		}
		else
			msg("yum, that tasted good");
	if (player.no_command)
		msg("You feel bloated and fall asleep");
}

/*
 * stomach:
 *	Digest the hero's food
 */
void
stomach()
{
	int oldfood, deltafood;
	rogue::Player &player = game().player;

	if (player.food_left <= 0)
	{
		if (player.food_left-- < -STARVETIME)
			death('s');
		/*
		 * the hero is fainting
		 */
		if (player.no_command || rnd(5) != 0)
			return;
		player.no_command += rnd(8) + 4;
		player.body.t_flags.unset(CreatureFlag::Running);
		game().turn.running = false;
		game().turn.count = 0;
		player.hungry_state = 3;
		msg("{}you faint from lack of food",noterse("you feel very weak. "));
	}
	else
	{
		oldfood = player.food_left;
		/*
		 * If you are in 40 column mode use food twice as fast
		 * (e.g. 3-(80/40) = 1, 3-(40/40) = 2 : pretty gross huh?)
		 */
		deltafood = ring_eat(Hand::Left) + ring_eat(Hand::Right) + 1;
		if (game().options.terse)
			deltafood *= 2;
		player.food_left -= deltafood;

		if (player.food_left < MORETIME && oldfood >= MORETIME)
		{
			player.hungry_state = 2;
			msg("you are starting to feel weak");
		}
		else if (player.food_left < 2 * MORETIME && oldfood >= 2 * MORETIME)
		{
			player.hungry_state = 1;
			msg("you are starting to get hungry");
		}
	}
}

}  // namespace rogue::rules
