#pragma once

#include "core/Random.hpp"
#include "rules/Scheduler.hpp"

/*
 * The state of one game, gathered from the globals of the original sources.
 *
 * Included by rogue.h after the legacy types it holds (Creature, Item,
 * struct room,
 * ...). Game files include rogue.h, not this header. Member names must not
 * collide with the lowercase macros of rogue.h and extern.h (hero, pack, max,
 * on, next, ...).
 */

namespace rogue {

/*
 * Player settings: read from rogue.opt (see persistence/OptionsFile), the
 * name prompt and the in-game toggles. The character buffers keep their
 * original sizes.
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
	unsigned char last_ch = 0;
	unsigned char last_take = 0;
	unsigned char do_take = 0;
	/* What get_item() last gave, so a repeat takes it again */
	unsigned char last_item_key = 0;			/* lch: its pack letter */
	Item *last_item = nullptr;		/* wasthing: the item itself */
};

/*
 * The rogue: the creature itself, what he carries and wears, his condition.
 */
struct Player {
	Creature body = {};				/* player: position, stats, flags, pack */
	struct stats max_stats = { 16, 0, 1, 10, 12, "1d4", 12 };	/* The maximum for the player */
	int purse = 0;					/* How much gold the rogue has */
	int in_pack = 0;				/* inpack: number of things in pack */
	Item *armor = nullptr;			/* cur_armor: what a well dresssed rogue wears */
	Item *weapon = nullptr;		/* cur_weapon: which weapon he is weilding */
	Item *rings[2] = {};			/* cur_ring: which rings are being worn */
	int food_left = 0;				/* Amount of food in hero's stomach */
	int hungry_state = 0;			/* How hungry is he */
	bool has_amulet = false;		/* amulet: he has the amulet */
	bool saw_amulet = false;		/* He has seen the amulet */
	int max_level = 0;				/* Deepest player has gone */
	int no_command = 0;				/* Number of turns asleep */
	int no_move = 0;				/* Number of turns held in place */
	int quiet = 0;					/* Number of quiet turns */
	int fung_hit = 0;				/* Number of time fungi has hit */
	char flytrap_damage[10] = "";	/* f_damage: the venus flytrap's attack, grows per hit */
	/*
	 * Not only a flag: a teleport trap makes it TRUE + 1 (be_trapped() in
	 * move.cpp), which look() in misc.cpp checks for.
	 */
	unsigned char was_trapped = FALSE;	/* Was a trap sprung */
	coord old_pos = {};				/* oldpos: position before last look() call */
	struct room *old_room = nullptr;	/* oldrp: roomin(&old_pos) */
};

/*
 * The level the rogue is on: its map, rooms, passages, and what lies and
 * lives on it.
 */
struct Level {
	int depth = 1;					/* level: what level rogue is on */
	int ntraps = 0;					/* Number of traps on this level */
	int no_food = 0;				/* Number of levels without food */
	struct room rooms[MAXROOMS] = {};	/* One for each room -- A level */
	struct room passages[MAXPASS] = {};	/* One for each passage */
	/*
	 * What is at each square, and its F_* flags. Index them with INDEX(y, x),
	 * or use chat()/flat().
	 */
	unsigned char map[(MAXLINES-3)*MAXCOLS] = {};	/* _level */
	unsigned char flags[(MAXLINES-3)*MAXCOLS] = {};	/* _flags */
	List<Item> objects;				/* lvl_obj: list of objects on this level */
	List<Creature> monsters;		/* mlist: list of monsters on the level */

	// Passages are dark rooms that are gone. The original table left the
	// 13th one lit by mistake.
	Level()
	{
		for (auto &p : passages)
			p.r_flags = RoomFlag::Gone | RoomFlag::Dark;
	}
};

/*
 * What there is to find in this game, how it looks, and what the rogue knows
 * about it.
 */
struct Items {
	/* Names, cumulative odds and worth of each kind; init_*() accumulate */
	struct magic_item s_magic[MAXSCROLLS];
	struct magic_item p_magic[MAXPOTIONS];
	struct magic_item r_magic[MAXRINGS];
	struct magic_item ws_magic[MAXSTICKS];
	struct magic_item things[NUMTHINGS];	/* Odds of each type of item */
	/* How the kinds look in this game */
	struct array s_names[MAXSCROLLS] = {};	/* Names of the scrolls */
	const char *p_colors[MAXPOTIONS] = {};	/* Colors of the potions */
	const char *r_stones[MAXRINGS] = {};	/* Stone settings of the rings */
	const char *ws_made[MAXSTICKS] = {};	/* What sticks are made of */
	const char *ws_type[MAXSTICKS] = {};	/* Is it a wand or a staff */
	/* What the rogue knows, and what he has called the kinds he doesn't */
	bool s_know[MAXSCROLLS] = {};			/* Does he know what a scroll does */
	bool p_know[MAXPOTIONS] = {};			/* Does he know what a potion does */
	bool r_know[MAXRINGS] = {};				/* Does he know what a ring does */
	bool ws_know[MAXSTICKS] = {};			/* Does he know what a stick does */
	char *s_guess[MAXSCROLLS] = {};			/* Players guess at what scroll is */
	char *p_guess[MAXPOTIONS] = {};			/* Players guess at what potion is */
	char *r_guess[MAXRINGS] = {};			/* Players guess at what ring is */
	char *ws_guess[MAXSTICKS] = {};			/* Players guess at what wand is */
	/* storage for the guesses (was _guesses) */
	struct array guesses[MAXSCROLLS+MAXPOTIONS+MAXRINGS+MAXSTICKS] = {};
	int iguess = 0;
	int group = 2;							/* Current group number */

	Items();
};

/*
 * The creatures and items in play, made by new_creature() and new_item() and
 * given back by discard() (list.cpp). Each slot owns its thing; lists, packs
 * and the rest only point at them. The original allocated both kinds from one
 * array of MAXITEMS things (_things), so the count is shared: when it is
 * full, neither kind can be made, and level generation checks it.
 */
struct Pool {
	Slots<Item, MAXITEMS> items;
	Slots<Creature, MAXITEMS> creatures;
	int total = 0;							/* Things of both kinds in use */
};

struct Game {
	Random random{Random::from_clock()};	/* All randomness, see rng() */
	Options options;
	Player player;
	Level level;
	Items items;
	Pool pool;
	rules::Scheduler scheduler;		/* Daemons and fuses */
	MessageLine message;
	Turn turn;
	bool playing = true;			/* True until he quits */
	bool noscore = false;			/* Only show the scores (-s), add none */
	int wander_rolls = 0;			/* between: rollwand() calls since it last rolled */

	Game() = default;
	// It points into itself (guesses, level lists, worn items in the pool)
	Game(const Game &) = delete;
	Game &operator=(const Game &) = delete;
};

/*
 * How the pool is referenced, as a list of problems (none when all is well):
 * each item in use is in exactly one of the level's objects, the rogue's
 * pack or a monster's pack; each creature in use is on the level's monster
 * list once; worn items are in the pack; the item get_item() gave last is in
 * use; a monster's t_dest is the hero, a room's or passage's gold, or a floor
 * item; rooms are rooms or passages; and the count of things in use is right.
 * Holds between commands, which is
 * when a game is saved.
 */
std::vector<std::string> pool_problems(const Game &g);

// The game being played.
Game &game();

// The generator of the game being played; rnd() and roll() use it.
inline Random &rng() { return game().random; }

}  // namespace rogue

using rogue::game;
