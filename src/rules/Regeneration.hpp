#pragma once

/*
 * Regaining hit points over time (see rules/Scheduler.hpp for when the
 * daemon runs).
 */

namespace rogue::rules {

/*
 * doctor:
 *	A healing daemon that restores hit points after rest.
 */
void doctor();

}  // namespace rogue::rules
