/*
 * The fuses that end a condition: confusion, seeing invisible, blindness
 * and haste.
 *
 * @(#)daemons.c	5.1 (Berkeley) 5/11/82
 */

#include "rules/Conditions.hpp"

#include "entities/Creature.hpp"
#include "entities/MonsterAI.hpp"
#include "game/Game.hpp"
#include "game/Messages.hpp"
#include "rules/Scheduler.hpp"
#include "ui/Display.hpp"
#include "world/Room.hpp"
#include "world/Rooms.hpp"

namespace rogue::rules {

/*
 * unconfuse:
 *	Release the poor player from his confusion
 */
void
unconfuse()
{
	game().player.body.flags.unset(CreatureFlag::Confused);
	msg("you feel less confused now");
}

/*
 * unsee:
 *	Turn off the ability to see invisible
 */
void
unsee()
{
	for (Creature &th : game().level.monsters)
		if (th.flags.test(CreatureFlag::Invisible) && entities::see_monst(th) && th.under != '@')
			ui::display().draw_tile(th.pos, th.under);
	game().player.body.flags.unset(CreatureFlag::SeeInvisible);
}

/*
 * sight:
 *	He gets his sight back
 */
void
sight()
{
	rogue::Player &player = game().player;

	if (player.body.flags.test(CreatureFlag::Blind))
	{
		extinguish(Event::Sight);
		player.body.flags.unset(CreatureFlag::Blind);
		if (!game().level.room(*player.body.room).flags.test(RoomFlag::Gone))
			world::enter_room(player.body.pos);
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
	game().player.body.flags.unset(CreatureFlag::Hasted);
	msg("you feel yourself slowing down");
}

}  // namespace rogue::rules
