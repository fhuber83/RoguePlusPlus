#pragma once

/*
 * Combat: the rogue attacking a monster and a monster attacking him, saving
 * throws, killing a monster and gaining experience levels.
 *
 * Included by rogue.h after the legacy types (str_t) and entities/Item.hpp
 * (Item)/Creature.hpp (Creature).
 */

namespace rogue {

class Item;
class Creature;

namespace rules {

/*
 * fight:
 *	The player attacks the monster (mn is its glyph on the map). Returns
 *	whether he hit it.
 */
bool fight(coord *mp, char mn, Item *weap, bool thrown);

/*
 * attack:
 *	The monster attacks the player.
 */
void attack(Creature *mp);

/*
 * swing:
 *	Returns true if the swing hits.
 */
bool swing(int at_lvl, int op_arm, int wplus);

/*
 * check_level:
 *	Check to see if the guy has gone up a level.
 */
void check_level();

/*
 * save_throw:
 *	See if a creature save against something.
 */
bool save_throw(int which, Creature *tp);

/*
 * save:
 *	See if he saves against various nasty things.
 */
bool save(int which);

/*
 * is_magic:
 *	Returns true if an object radiates magic.
 */
bool is_magic(Item *obj);

/*
 * raise_level:
 *	The guy just magically went up a level.
 */
void raise_level();

/*
 * killed:
 *	Called to put a monster to death.
 */
void killed(Creature *tp, bool pr);

}  // namespace rules
}  // namespace rogue
