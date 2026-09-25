#pragma once

/*
 * Rings: putting one on, taking one off, its food cost, and its bonus
 * string for the inventory name.
 */

namespace rogue {

class Item;

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
 * ring_eat:
 *	How much food does the ring on this hand use up?
 */
int ring_eat(int hand);

/*
 * ring_num:
 *	Print ring bonuses.
 */
std::string ring_num(const Item *obj);

}  // namespace items::effects
}  // namespace rogue
