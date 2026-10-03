#pragma once

/*
 * Rings: putting one on, taking one off, its food cost, and its bonus
 * string for the inventory name.
 */

#include <string>

namespace rogue {

struct Item;
enum class Hand;

namespace items::effects {

/*
 * ring_on:
 *	Put a ring on a hand.
 */
void ring_on();

/*
 * ring_off:
 *	Take off a ring.
 */
void ring_off();

/*
 * ring_num:
 *	Print ring bonuses.
 */
std::string ring_num(const Item &obj);

}  // namespace items::effects
}  // namespace rogue
