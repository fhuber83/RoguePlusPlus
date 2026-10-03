/*
 * Wandering monsters: the fuse and daemon that bring them.
 *
 * @(#)daemons.c	5.1 (Berkeley) 5/11/82
 */

#include "rules/Wandering.hpp"

#include "entities/MonsterCatalog.hpp"
#include "game/Game.hpp"
#include "rules/Durations.hpp"
#include "rules/Scheduler.hpp"

namespace rogue::rules {

/*
 * Swander:
 *	Called when it is time to start rolling for wandering monsters
 */
void
swander()
{
	start_daemon(Event::RollWander);
}

/*
 * rollwand:
 *	Called to roll to see if a wandering monster starts up
 */
void
rollwand()
{
	int &between = game().wander_rolls;

	if (++between >= 3 + rnd(3))
	{
		if (roll(1, 6) == 4)
		{
			entities::wanderer();
			extinguish(Event::RollWander);
			fuse(Event::Swander, wander_time());
		}
	between = 0;
	}
}

}  // namespace rogue::rules
