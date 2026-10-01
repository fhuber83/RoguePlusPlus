#pragma once

#include <string>
#include <string_view>

/*
 * Setting up a new game: the rogue's name, his first pack, and the odds and
 * looks of the kinds of item for this game. main() calls credits(), the
 * init_*() functions in the order below, and setup(), after reading the
 * options and seeding rng(); each draw from rng() is part of the dungeon a
 * seed makes.
 */

namespace rogue {

/*
 * setup:
 *	Start with the terse and expert toggles off (a restored game then
 *	takes its own from the save).
 */
void setup();

/*
 * credits:
 *	Show the title screen and ask for the rogue's name, which replaces
 *	the one from rogue.opt unless the player only presses Enter or Escape.
 */
void credits();

/*
 * init_player:
 *	Roll up the rogue: his stats and food, an empty pool, and his mace,
 *	bow, arrows, ring mail and a ration.
 */
void init_player();

/*
 * init_things:
 *	Make the odds of each type of item cumulative.
 */
void init_things();

/*
 * init_names:
 *	Make up the titles of the scrolls, and make their odds cumulative.
 */
void init_names();

/*
 * init_colors:
 *	Give each potion a color, and make their odds cumulative.
 */
void init_colors();

/*
 * init_stones:
 *	Set each ring with a stone, which adds to its worth, and make their
 *	odds cumulative.
 */
void init_stones();

/*
 * init_materials:
 *	Make each stick a wand of some metal or a staff of some wood, and make
 *	their odds cumulative.
 */
void init_materials();

/*
 * getsyl:
 *	A random syllable of a scroll title: consonant, vowel, consonant.
 */
std::string getsyl();

/*
 * rchr:
 *	A random character of a string.
 */
char rchr(std::string_view string);

}  // namespace rogue
