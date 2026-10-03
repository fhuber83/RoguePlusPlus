#pragma once

#include "core/Dice.hpp"

namespace rogue::entities {

/*
 * Data type for strength values and modifiers
 */
using str_t = unsigned int;

/*
 * What a fighting being is made of (was struct stats): the rogue's, a
 * monster's, and a kind of monster's to start with.
 */
struct Stats {
	str_t str;			/* Strength */
	long exp;				/* Experience */
	int level;			/* Level of mastery */
	int armor;			/* Armor class */
	int hp;			/* Hit points */
	Attacks damage;		/* Damage done, per attack */
	int max_hp;			/* Max hit points */
};

}  // namespace rogue::entities
