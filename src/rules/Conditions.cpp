/*
 * The fuses that end a condition: confusion, seeing invisible, blindness
 * and haste.
 *
 * @(#)daemons.c	5.1 (Berkeley) 5/11/82
 */

#include "rogue.h"

namespace rogue::rules {

/*
 * unconfuse:
 *	Release the poor player from his confusion
 */
void
unconfuse()
{
	game().player.body.t_flags.unset(ISHUH);
	msg("you feel less confused now");
}

/*
 * unsee:
 *	Turn off the ability to see invisible
 */
void
unsee()
{
	Maybe<Creature> th;

	for (th = game().level.monsters.first(); th; th = game().level.monsters.after(*th))
		if (th->t_flags.test(ISINVIS) && see_monst(*th) && th->t_oldch != '@')
			display().draw_tile(th->t_pos, th->t_oldch);
	game().player.body.t_flags.unset(CANSEE);
}

/*
 * sight:
 *	He gets his sight back
 */
void
sight()
{
	rogue::Player &player = game().player;

	if (player.body.t_flags.test(ISBLIND))
	{
		extinguish(Event::Sight);
		player.body.t_flags.unset(ISBLIND);
		if (!game().level.room(*player.body.t_room).r_flags.test(RoomFlag::Gone))
			enter_room(player.body.t_pos);
		msg("the veil of darkness lifts");
	}
}

/*
 * nohaste:
 *	End the hasting
 */
void
nohaste()
{
	game().player.body.t_flags.unset(ISHASTE);
	msg("you feel yourself slowing down");
}

}  // namespace rogue::rules
