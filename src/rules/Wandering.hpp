#pragma once

/*
 * When wandering monsters come (see rules/Scheduler.hpp for when these
 * run; entities::wanderer() makes the monster).
 */

namespace rogue::rules {

/*
 * swander:
 *	Called when it is time to start rolling for wandering monsters.
 */
void swander();

/*
 * rollwand:
 *	Called to roll to see if a wandering monster starts up.
 */
void rollwand();

}  // namespace rogue::rules
