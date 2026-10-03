#pragma once

#include "core/Dice.hpp"

namespace rogue::entities {

/*
 * Data type for strength values and modifiers
 */
using str_t = unsigned int;

// Add amt to a strength, keeping it between 3 and 31 (was add_str()). As in
// the original, str_t is unsigned, so a strength taken below 0 wraps and
// ends at 31; in play it never comes to that, as strength is at least 3
// and is taken a few points at a time.
constexpr void
add_str(str_t &sp, int amt)
{
	if ((sp += amt) < 3)
		sp = 3;
	else if (sp > 31)
		sp = 31;
}

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
