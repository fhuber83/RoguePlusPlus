#pragma once

/*
 * Armor: putting it on and taking it off.
 */

namespace rogue::items::effects {

/*
 * wear:
 *	The player wants to wear something, so let him/her put it on.
 */
void wear();

/*
 * take_off:
 *	Get the armor off of the player's back.
 */
void take_off();

/*
 * waste_time:
 *	Do nothing but let other things (daemons, fuses) happen.
 */
void waste_time();

}  // namespace rogue::items::effects
