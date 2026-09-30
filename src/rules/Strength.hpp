#pragma once

/*
 * The rogue's strength: changing it within its bounds, and keeping track of
 * the highest it has been.
 *
 * Included by rogue.h after the legacy types (str_t).
 */

namespace rogue::rules {

/*
 * chg_str:
 *	Change the rogue's strength by amt, and raise the maximum if his
 *	strength without rings of strength is higher.
 */
void chg_str(int amt);

/*
 * add_str:
 *	Add amt to a strength, keeping it between 3 and 31.
 */
void add_str(str_t &sp, int amt);

}  // namespace rogue::rules
