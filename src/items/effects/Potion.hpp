#pragma once

/*
 * Potions: quaffing one, throwing one at a monster, and the invisibility
 * and monster-sensing state they turn on.
 *
 * Included by rogue.h after extern.h (byte) and entities/Item.hpp
 * (Item)/Creature.hpp (Creature).
 */

namespace rogue {

class Item;
class Creature;

namespace items::effects {

/*
 * quaff:
 *	Quaff a potion from the pack.
 */
void quaff();

/*
 * invis_on:
 *	Turn on the ability to see invisible.
 */
void invis_on();

/*
 * turn_see:
 *	Put on or off seeing monsters on this level. Returns whether a
 *	previously-unseen monster was added to the sensed set.
 */
bool turn_see(bool turn_off);

/*
 * th_effect:
 *	Compute the effect of a thrown potion hitting a monster.
 */
void th_effect(Item *obj, Creature *tp);

}  // namespace items::effects
}  // namespace rogue
