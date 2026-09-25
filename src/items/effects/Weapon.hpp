#pragma once

/*
 * Weapons: throwing one, wielding one, and what happens to an item that
 * lands on the floor.
 *
 * Included by rogue.h after entities/Item.hpp (Item).
 */

namespace rogue {

class Item;

namespace items::effects {

/*
 * missile:
 *	Fire a missile in a given direction.
 */
void missile(int ydelta, int xdelta);

/*
 * do_motion:
 *	Do the actual motion on the screen done by an object traveling
 *	across the room.
 */
void do_motion(Item *obj, int ydelta, int xdelta);

/*
 * fall:
 *	Drop an item someplace around here.
 */
void fall(Item *obj, bool pr);

/*
 * init_weapon:
 *	Set up the initial goodies for a weapon.
 */
void init_weapon(Item *weap, unsigned char type);

/*
 * hit_monster:
 *	Does the missile hit the monster?
 */
bool hit_monster(int y, int x, Item *obj);

/*
 * num:
 *	Figure out the plus number for armor/weapons.
 */
std::string num(int n1, int n2, char type);

/*
 * wield:
 *	Pull out a certain weapon.
 */
void wield();

/*
 * tick_pause:
 *	Pause for a tick, ie, 1/18.2 secs (about 55ms).
 */
void tick_pause();

}  // namespace items::effects
}  // namespace rogue
