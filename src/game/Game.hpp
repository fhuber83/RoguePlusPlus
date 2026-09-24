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

/*
 * The message line: the message being built, the one shown and the last one
 * kept for ^R.
 */
struct MessageLine {
	char text[BUFSIZE] = "";		/* msgbuf: the message being built */
	char last[BUFSIZE] = "";		/* huh: the last message printed */
	int end = 0;					/* mpos: where the shown message ends, 0 if none */
	int next_end = 0;				/* newpos: where the message being built ends */
	bool remember = true;			/* save_msg: keep the message for ^R */
};

/*
 * The command being carried out: repeat counts, running, and the keys a
 * macro still has to type.
 */
struct Turn {
	bool after = false;				/* True if we want after daemons */
	bool again = false;				/* The last command is repeated */
	int count = 0;					/* Number of times to repeat command */
	char take = 0;					/* Thing the rogue is taking */
	bool running = false;			/* True if player is running */
	char run_dir = 0;				/* runch: direction player is running */
	bool door_stop = false;			/* Stop running when we pass a door */
	bool first_move = false;		/* First move after setting door_stop */
	bool fast_mode = false;			/* Run until you see something */
	bool fast_state = false;		/* Toggle for find (see above) */
	coord delta = {};				/* Change indicated to get_dir() */
	const char *typeahead = "";		/* typebuf: keys a macro still types */
	bool bailout = false;			/* The hero is nowhere: fall through */
	/* What the last command was, for repeating it (command.cpp) */
	int last_count = 0;
	byte last_ch = 0;
	byte last_take = 0;
	byte do_take = 0;
};

struct Game {
	Options options;
	MessageLine message;
	Turn turn;
	bool playing = true;			/* True until he quits */
	bool noscore = false;			/* Was a wizard sometime */
};

// The game being played.
Game &game();

}  // namespace rogue

using rogue::game;
