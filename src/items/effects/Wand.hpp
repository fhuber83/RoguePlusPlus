#pragma once

/*
 * Sticks (wands and staves): setting one up, zapping it, and the bolt it
 * fires.
 *
 * Included by rogue.h after core/Coord.hpp (coord) and entities/Item.hpp
 * (Item).
 */

namespace rogue {

class Item;

namespace items::effects {

/*
 * fix_stick:
 *	Set up a new stick.
 */
void fix_stick(Item *cur);

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
void fire_bolt(coord *start, coord *dir, const char *name);

/*
 * charge_str:
 *	Return an appropriate string for a wand's charge count.
 */
char *charge_str(Item *obj);

}  // namespace items::effects
}  // namespace rogue
