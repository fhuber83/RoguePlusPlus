/*
 * All the daemon and fuse functions are in here
 *
 * @(#)daemons.c	5.1 (Berkeley) 5/11/82
 */

#include "rogue.h"


/*
 * doctor:
 *	A healing daemon that restores hit points after rest
 */
void
doctor(void)
{
	int lv, ohp;

	lv = pstats.s_lvl;
	ohp = pstats.s_hpt;
	game().player.quiet++;
	if (lv < 8)
	{
		if (game().player.quiet + (lv << 1) > 20)
			pstats.s_hpt++;
	}
	else
	if (game().player.quiet >= 3)
		pstats.s_hpt += rnd(lv - 7) + 1;
	if (ISRING(LEFT, R_REGEN))
		pstats.s_hpt++;
	if (ISRING(RIGHT, R_REGEN))
		pstats.s_hpt++;
	if (ohp != pstats.s_hpt)
	{
		if (pstats.s_hpt > max_hp)
			pstats.s_hpt = max_hp;
		game().player.quiet = 0;
	}
}

/*
 * Swander:
 *	Called when it is time to start rolling for wandering monsters
 */
void
swander(void)
{
	start_daemon(rollwand);
}

/*
 * rollwand:
 *	Called to roll to see if a wandering monster starts up
 */
void
rollwand(void)
{
	static int between = 0;

	if (++between >= 3 + rnd(3))
	{
		if (roll(1, 6) == 4)
		{
			wanderer();
			extinguish(rollwand);
			fuse(swander, WANDERTIME);
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
		if (on(*th, ISINVIS) && see_monst(th) && th->t_oldch != '@')
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
	if (on(game().player.body, ISBLIND))
	{
		extinguish(sight);
		game().player.body.t_flags.unset(ISBLIND);
		if (!proom->r_flags.test(RoomFlag::Gone))
			enter_room(&hero);
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
		msg("%syou faint from lack of food",noterse("you feel very weak. "));
	}
	else
	{
		oldfood = player.food_left;
		/*
		 * If you are in 40 column mode use food twice as fast
		 * (e.g. 3-(80/40) = 1, 3-(40/40) = 2 : pretty gross huh?)
		 */
		deltafood = ring_eat(LEFT) + ring_eat(RIGHT) + 1;
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
