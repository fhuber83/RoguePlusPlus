/*
 * Eating.
 *
 * eat() comes from misc.c.
 *
 * misc.c	1.4		(A.I. Design)	12/14/84
 */

#include "rogue.h"

namespace rogue::rules {

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

}  // namespace rogue::rules
