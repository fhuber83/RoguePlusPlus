#pragma once

#include "core/Dice.hpp"

namespace rogue::entities {

/*
 * Data type for strength values and modifiers
 */
using str_t = unsigned int;

/*
 * What a fighting being is made of (was Stats): the rogue's, a
 * monster's, and a kind of monster's to start with.
 */
struct Stats {
	str_t s_str;			/* Strength */
	long s_exp;				/* Experience */
	int s_lvl;			/* Level of mastery */
	int s_arm;			/* Armor class */
	int s_hpt;			/* Hit points */
	Attacks s_dmg;		/* Damage done, per attack */
	int s_maxhp;			/* Max hit points */
};

}  // namespace rogue::entities
