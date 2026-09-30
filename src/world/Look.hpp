#pragma once

/*
 * What the rogue sees and finds around it: the quick glance after every
 * move, and searching for hidden doors and traps.
 */

namespace rogue::world {

/*
 * look:
 *	A quick glance all around the rogue: redraws the squares next to him,
 *	wakes what he sees if wakeup is set, and stops a run where there is
 *	something to stop for.
 */
void look(bool wakeup);

/*
 * search:
 *	The rogue gropes about him to find hidden doors and traps.
 */
void search();

}  // namespace rogue::world
