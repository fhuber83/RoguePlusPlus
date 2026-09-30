#pragma once

/*
 * Eating. (The stomach daemon is rules/Daemons' stomach().)
 */

namespace rogue::rules {

/*
 * eat:
 *	The rogue eats something from the pack: a ration or a fruit fills the
 *	stomach, and anything else is refused.
 */
void eat();

}  // namespace rogue::rules
