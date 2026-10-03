#pragma once

/*
 * Sticks (wands and staves): setting one up, zapping it, and the bolt it
 * fires.
 */

#include <string>
#include <string_view>

#include "core/Coord.hpp"

namespace rogue {

struct Item;

namespace items::effects {

inline constexpr int BOLT_LENGTH = 6;	/* squares a bolt flies */

/*
 * fix_stick:
 *	Set up a new stick.
 */
void fix_stick(Item &cur);

/*
 * do_zap:
 *	Perform a zap with a wand.
 */
void do_zap();

/*
 * drain:
 *	Drain hit points from the player to the monsters around them (staff
 *	of draining).
 */
void drain();

/*
 * fire_bolt:
 *	Fire a bolt in a given direction from a specific starting place.
 */
void fire_bolt(Coord start, Coord &dir, std::string_view name);

/*
 * charge_str:
 *	Return an appropriate string for a wand's charge count.
 */
std::string charge_str(const Item &obj);

}  // namespace items::effects
}  // namespace rogue
