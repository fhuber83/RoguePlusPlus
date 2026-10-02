#pragma once

/*
 * The status line and the clock: what the game shows of the rogue at the
 * bottom of the screen. The display draws them (ui::Status, draw_clock).
 */

namespace rogue {

/*
 * status:
 *	Show the rogue's level, hit points, strength, gold, armor, rank and
 *	hunger, and the clock.
 */
void status();

/*
 * SIG2:
 *	Show the clock, when its minute has changed and no page is open.
 */
void SIG2();

}  // namespace rogue
