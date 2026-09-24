#pragma once

/*
 * The state of one game, gathered from the globals of the original sources.
 *
 * Included by rogue.h after the legacy types it holds (THING, struct room,
 * ...). Game files include rogue.h, not this header. Member names must not
 * collide with the lowercase macros of rogue.h and extern.h (hero, pack, max,
 * on, next, ...).
 */

namespace rogue {

/*
 * Player settings: read from rogue.opt (see env.cpp), the name prompt and
 * the in-game toggles. The character buffers keep their original sizes,
 * which env.cpp relies on.
 */
struct Options {
	char name[24] = "Rodney";		/* whoami: the rogue's name */
	char fruit[24] = "Slime Mold";	/* What the player likes to eat */
	char macro[42] = "v";			/* Keys the F9 macro types */
	char score_file[15] = "rogue.scr";
	char save_file[15] = "rogue.sav";
	char drive[2] = "?";			/* Unused DOS drive letter */
	char menu[4] = "on";			/* Item menus: "on", "sel" or off */
	char screen[8] = "";			/* "bw" forces monochrome */
	bool monochrome = false;		/* Draw without colours (bwflag) */
	bool terse = false;				/* Short messages */
	bool expert = false;			/* Even shorter messages */

	// Leave out the flavour text of messages
	bool brief() const { return terse || expert; }
};

struct Game {
	Options options;
};

// The game being played.
Game &game();

}  // namespace rogue

using rogue::game;
