#pragma once

/*
 * Eating, and digesting: the stomach daemon.
 */

namespace rogue::rules {

/*
 * eat:
 *	The rogue eats something from the pack: a ration or a fruit fills the
 *	stomach, and anything else is refused.
 */
void eat();

/*
 * stomach:
 *	Digest the hero's food.
 */
void stomach();

}  // namespace rogue::rules
