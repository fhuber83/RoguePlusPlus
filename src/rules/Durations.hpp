#pragma once

/*
 * How long things last, each spread by 10% (were BEARTIME, SLEEPTIME, ...).
 */

#include "game/Game.hpp"

namespace rogue::rules {

inline int bear_time() { return spread(3); }		/* held by a bear trap */
inline int sleep_time() { return spread(5); }		/* asleep from a gas trap or scroll */
inline int hold_time() { return spread(2); }		/* paralyzed by a potion */
inline int wander_time() { return spread(70); }	/* until the next wandering monster */
inline int huh_duration() { return spread(20); }	/* confused */
inline int see_duration() { return spread(300); }	/* seeing invisible, or blind */
inline int hunger_time() { return spread(1300); }	/* a full stomach */

}  // namespace rogue::rules
