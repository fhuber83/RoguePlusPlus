#pragma once

/*
 * Combat: the rogue attacking a monster and a monster attacking him, saving
 * throws, killing a monster and gaining experience levels.
 */

#include "core/Coord.hpp"
#include "core/Maybe.hpp"
#include "entities/Stats.hpp"

namespace rogue {

struct Item;
struct Creature;

namespace rules {

/*
 * What a saving throw is against (were the VS_* defines). The number is added
 * to what the throw needs, so several share one.
 */
enum class SaveThrow {
	Poison = 0,
	Paralyzation = 0,
	Death = 0,
	Luck = 1,
	Breath = 2,
	Magic = 3,
};

/*
 * fight:
 *	The player attacks the monster (mn is its glyph on the map). Returns
 *	whether he hit it.
 */
bool fight(Coord mp, char mn, Maybe<Item> weap, bool thrown);

/*
 * attack:
 *	The monster attacks the player.
 */
void attack(Creature &mp);

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
bool save_throw(SaveThrow which, const Creature &tp);

/*
 * save:
 *	See if he saves against various nasty things.
 */
bool save(SaveThrow which);

/*
 * raise_level:
 *	The guy just magically went up a level.
 */
void raise_level();

/*
 * killed:
 *	Called to put a monster to death.
 */
void killed(Creature &tp, bool pr);

}  // namespace rules
}  // namespace rogue
