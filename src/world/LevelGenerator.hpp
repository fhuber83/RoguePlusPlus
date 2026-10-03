#pragma once

/*
 * Making a new level: rooms, passages and mazes, the stairs, the things
 * and monsters on it, and sometimes a treasure room.
 */

#include "game/Game.hpp"

namespace rogue::world {

// The level the amulet is on, and below which no level is made without it
inline constexpr int AMULETLEVEL = 26;

// The gold in a pile on this level (was GOLDCALC)
inline int gold_calc() { return rnd(50 + 10 * game().level.depth) + 2; }

/*
 * new_level:
 *	Dig and draw a new level.
 */
void new_level();

/*
 * rnd_room:
 *	Pick a room that is really there.
 */
int rnd_room();

}  // namespace rogue::world
