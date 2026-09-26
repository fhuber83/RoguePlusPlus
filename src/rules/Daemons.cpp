/*
 * All the daemon and fuse functions are in here
 *
 * @(#)daemons.c	5.1 (Berkeley) 5/11/82
 */

#include "rogue.h"

namespace rogue::rules {

/*
 * doctor:
 *	A healing daemon that restores hit points after rest
 */
void
doctor(void)
{
	int lv, ohp;
	rogue::Player &player = game().player;

	lv = player.body.t_stats.s_lvl;
	ohp = player.body.t_stats.s_hpt;
	player.quiet++;
	if (lv < 8)
	{
		if (player.quiet + (lv << 1) > 20)
			player.body.t_stats.s_hpt++;
	}
	else
	if (player.quiet >= 3)
		player.body.t_stats.s_hpt += rnd(lv - 7) + 1;
	if (player.wears(Hand::Left, Ring::Regeneration))
		player.body.t_stats.s_hpt++;
	if (player.wears(Hand::Right, Ring::Regeneration))
		player.body.t_stats.s_hpt++;
	if (ohp != player.body.t_stats.s_hpt)
	{
		if (player.body.t_stats.s_hpt > player.body.t_stats.s_maxhp)
			player.body.t_stats.s_hpt = player.body.t_stats.s_maxhp;
		player.quiet = 0;
	}
}

/*
 * Swander:
 *	Called when it is time to start rolling for wandering monsters
 */
void
swander(void)
{
	start_daemon(Event::RollWander);
}

/*
 * rollwand:
 *	Called to roll to see if a wandering monster starts up
 */
void
rollwand(void)
{
	int &between = game().wander_rolls;

	if (++between >= 3 + rnd(3))
	{
		if (roll(1, 6) == 4)
		{
			wanderer();
			extinguish(Event::RollWander);
			fuse(Event::Swander, wander_time());
		}
	between = 0;
	}
}

/*
 * unconfuse:
 *	Release the poor player from his confusion
 */
void
unconfuse(void)
{
	game().player.body.t_flags.unset(ISHUH);
	msg("you feel less confused now");
}

/*
 * unsee:
 *	Turn off the ability to see invisible
 */
void
unsee(void)
{
	Creature *th;

	for (th = game().level.monsters.first(); th != NULL; th = game().level.monsters.after(th))
		if (th->t_flags.test(ISINVIS) && see_monst(th) && th->t_oldch != '@')
			display().draw_tile(th->t_pos, th->t_oldch);
	game().player.body.t_flags.unset(CANSEE);
}

/*
 * sight:
 *	He gets his sight back
 */
void
sight(void)
{
	rogue::Player &player = game().player;

	if (player.body.t_flags.test(ISBLIND))
	{
		extinguish(Event::Sight);
		player.body.t_flags.unset(ISBLIND);
		if (!player.body.t_room->r_flags.test(RoomFlag::Gone))
			enter_room(&player.body.t_pos);
		msg("the veil of darkness lifts");
	}
}

/*
 * nohaste:
 *	End the hasting
 */
void
nohaste(void)
{
	game().player.body.t_flags.unset(ISHASTE);
	msg("you feel yourself slowing down");
}

/*
 * stomach:
 *	Digest the hero's food
 */
void
stomach(void)
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
		player.body.t_flags.unset(ISRUN);
		game().turn.running = FALSE;
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
