#pragma once

/*
 * The rogue's strength: changing it within its bounds, and keeping track of
 * the highest it has been.
 */

#include "entities/Stats.hpp"

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
void add_str(entities::str_t &sp, int amt);

}  // namespace rogue::rules
