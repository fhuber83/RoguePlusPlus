/*
 * The fuses that end a condition: confusion, seeing invisible, blindness
 * and haste.
 *
 * @(#)daemons.c	5.1 (Berkeley) 5/11/82
 */

#include "rules/Conditions.hpp"

#include "core/Maybe.hpp"
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
	game().player.body.t_flags.unset(CreatureFlag::Confused);
	msg("you feel less confused now");
}

/*
 * unsee:
 *	Turn off the ability to see invisible
 */
void
unsee()
{
	for (Maybe<Creature> th = game().level.monsters.first(); th; th = game().level.monsters.after(*th))
		if (th->t_flags.test(CreatureFlag::Invisible) && entities::see_monst(*th) && th->t_oldch != '@')
			ui::display().draw_tile(th->t_pos, th->t_oldch);
	game().player.body.t_flags.unset(CreatureFlag::SeeInvisible);
}

/*
 * sight:
 *	He gets his sight back
 */
void
sight()
{
	rogue::Player &player = game().player;

	if (player.body.t_flags.test(CreatureFlag::Blind))
	{
		extinguish(Event::Sight);
		player.body.t_flags.unset(CreatureFlag::Blind);
		if (!game().level.room(*player.body.t_room).r_flags.test(RoomFlag::Gone))
			world::enter_room(player.body.t_pos);
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
	game().player.body.t_flags.unset(CreatureFlag::Hasted);
	msg("you feel yourself slowing down");
}

}  // namespace rogue::rules
