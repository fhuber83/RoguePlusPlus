#pragma once

/*
 * The state of one game, gathered from the globals of the original sources.
 */

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "core/Coord.hpp"
#include "core/Glyphs.hpp"
#include "core/KindTable.hpp"
#include "core/Maybe.hpp"
#include "core/Random.hpp"
#include "entities/Creature.hpp"
#include "entities/Item.hpp"
#include "entities/List.hpp"
#include "entities/Stats.hpp"
#include "game/Id.hpp"
#include "game/Slots.hpp"
#include "items/KindInfo.hpp"
#include "items/Kinds.hpp"
#include "rules/Scheduler.hpp"
#include "world/Map.hpp"
#include "world/MapFlags.hpp"
#include "world/Room.hpp"
#include "world/RoomRef.hpp"

namespace rogue {

/*
 * Player settings: read from rogue.opt (see persistence/OptionsFile), the
 * name prompt and the in-game toggles. The texts keep the longest lengths
 * of the original buffers.
 */
struct Options {
	static constexpr std::size_t name_length = 23;		/* also the fruit's */
	static constexpr std::size_t macro_length = 40;
	static constexpr std::size_t file_length = 14;		/* the score and save files' */

	std::string name = "Rodney";		/* whoami: the rogue's name */
	std::string fruit = "Slime Mold";	/* What the player likes to eat */
	std::string macro = "v";			/* Keys the F9 macro types */
	std::string score_file = "rogue.scr";
	std::string save_file = "rogue.sav";
	std::string drive = "?";			/* Unused DOS drive letter */
	std::string menu = "on";			/* Item menus: "on", "sel" or off */
	std::string screen = "";			/* "bw" forces monochrome */
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
inline constexpr int BUFSIZE = 128;	/* the longest message, with its end */

struct MessageLine {
	std::string text;				/* msgbuf: the message being built, at most BUFSIZE - 1 */
	std::string last;				/* huh: the last message printed */
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
	Coord delta = {};				/* Change indicated to get_dir() */
	std::string typeahead;			/* typebuf: keys a macro still types */
	bool bailout = false;			/* The hero is nowhere: fall through */
	int moves_left = 0;				/* ntimes: moves left in this command (2 or 3 when hasted) */
	/* What the last command was, for repeating it (command.cpp) */
	int last_count = 0;
	unsigned char last_ch = 0;
	unsigned char last_take = 0;
	unsigned char do_take = 0;
	/* What get_item() last gave, so a repeat takes it again */
	unsigned char last_item_key = 0;			/* lch: its pack letter */
	std::optional<ItemId> last_item;	/* wasthing: the item itself */
};

/*
 * What the last trap did, for look() to show (was_trapped). The original
 * counted it past TRUE: a teleport trap made it TRUE + 1.
 */
enum class Trapped : unsigned char {
	None,
	Sprung,			/* a trap was sprung */
	Teleported,		/* a teleport trap moved the rogue */
};

/*
 * The rogue: the creature itself, what he carries and wears, his condition.
 */
struct Player {
	Creature body = {};				/* player: position, stats, flags, pack */
	entities::Stats max_stats = { 16, 0, 1, 10, 12, "1d4", 12 };	/* The maximum for the player */
	int purse = 0;					/* How much gold the rogue has */
	int in_pack = 0;				/* inpack: number of things in pack */
	std::optional<ItemId> armor;		/* cur_armor: what a well dresssed rogue wears */
	std::optional<ItemId> weapon;		/* cur_weapon: which weapon he is weilding */
	KindTable<Hand, std::optional<ItemId>> rings = {};	/* cur_ring: which rings are being worn */
	int food_left = 0;				/* Amount of food in hero's stomach */
	int hungry_state = 0;			/* How hungry is he */
	bool has_amulet = false;		/* amulet: he has the amulet */
	bool saw_amulet = false;		/* He has seen the amulet */
	int max_level = 0;				/* Deepest player has gone */
	int no_command = 0;				/* Number of turns asleep */
	int no_move = 0;				/* Number of turns held in place */
	int quiet = 0;					/* Number of quiet turns */
	int fung_hit = 0;				/* Number of times the venus flytrap has hit; its attack is fung_hit d1 */
	Trapped was_trapped = Trapped::None;	/* Was a trap sprung (be_trapped(), look()) */
	Coord old_pos = {};				/* oldpos: position before last look() call */
	std::optional<RoomRef> old_room;	/* oldrp: roomin(old_pos) */

	// What he wears and wields, if anything (these look it up in game().pool)
	Maybe<Item> armor_item() const;
	Maybe<Item> weapon_item() const;
	Maybe<Item> ring_item(Hand hand) const;
	// Whether he wears this ring on this hand (was ISRING)
	bool wears(Hand hand, Ring ring) const;
	// Whether he wears this ring on either hand (was ISWEARING)
	bool wears(Ring ring) const { return wears(Hand::Left, ring) || wears(Hand::Right, ring); }
};

/*
 * The level the rogue is on: its map, rooms, passages, and what lies and
 * lives on it.
 */
struct Level {
	int depth = 1;					/* level: what level rogue is on */
	int ntraps = 0;					/* Number of traps on this level */
	int no_food = 0;				/* Number of levels without food */
	std::array<world::Room, world::MAXROOMS> rooms = {};	/* One for each room -- A level */
	std::array<world::Room, world::MAXPASS> passages = {};	/* One for each passage */
	/*
	 * What is at each square, and its MapFlags. Index them with INDEX(y, x),
	 * or use at()/flags_at().
	 */
	unsigned char map[(MAXLINES-3)*MAXCOLS] = {};	/* _level */
	MapFlags flags[(MAXLINES-3)*MAXCOLS] = {};	/* _flags */
	List<Item> objects;				/* lvl_obj: list of objects on this level */
	List<Creature> monsters;		/* mlist: list of monsters on the level */

	// Passages are dark rooms that are gone. The original table left the
	// 13th one lit by mistake.
	Level()
	{
		for (auto &p : passages)
			p.r_flags = RoomFlag::Gone | RoomFlag::Dark;
	}

	// What is at a square (was chat())
	unsigned char &at(int y, int x) { return map[world::INDEX(y, x)]; }
	unsigned char &at(Coord pos) { return at(pos.y, pos.x); }
	// A square's MapFlags (was flat())
	MapFlags &flags_at(int y, int x) { return flags[world::INDEX(y, x)]; }
	MapFlags &flags_at(Coord pos) { return flags_at(pos.y, pos.x); }
	// The room or passage a RoomRef names
	world::Room &room(RoomRef r) { return r.kind == RoomRef::Kind::Room ? rooms[r.index] : passages[r.index]; }
	const world::Room &room(RoomRef r) const
	{
		return r.kind == RoomRef::Kind::Room ? rooms[r.index] : passages[r.index];
	}
	// Whether a RoomRef names one of this level's rooms or passages
	static constexpr bool valid(RoomRef r)
	{
		return r.index >= 0 && r.index < (r.kind == RoomRef::Kind::Room ? world::MAXROOMS : world::MAXPASS);
	}
	// The passage a passage or maze square belongs to
	RoomRef passage_at(Coord pos) { return RoomRef::passage(flags_at(pos).passage()); }
};

/*
 * What there is to find in this game, how it looks, and what the rogue knows
 * about it.
 */
inline constexpr int MAXNAME = 20;	/* the longest name he calls a kind */

struct Items {
	/* Names, cumulative odds and worth of each kind; init_*() accumulate */
	KindTable<Scroll, items::KindInfo> s_magic;
	KindTable<Potion, items::KindInfo> p_magic;
	KindTable<Ring, items::KindInfo> r_magic;
	KindTable<Stick, items::KindInfo> ws_magic;
	std::array<items::KindInfo, items::NUMTHINGS> things;	/* Odds of each type of item */
	/* How the kinds look in this game */
	KindTable<Scroll, std::string> s_names;	/* Names of the scrolls */
	KindTable<Potion, std::string_view> p_colors = {};	/* Colors of the potions */
	KindTable<Ring, std::string_view> r_stones = {};	/* Stone settings of the rings */
	KindTable<Stick, std::string_view> ws_made = {};	/* What sticks are made of */
	KindTable<Stick, std::string_view> ws_type = {};	/* Is it a wand or a staff */
	/* What the rogue knows, and what he has called the kinds he doesn't */
	KindTable<Scroll, bool> s_know = {};			/* Does he know what a scroll does */
	KindTable<Potion, bool> p_know = {};			/* Does he know what a potion does */
	KindTable<Ring, bool> r_know = {};				/* Does he know what a ring does */
	KindTable<Stick, bool> ws_know = {};			/* Does he know what a stick does */
	/* What he has called each kind, at most MAXNAME characters; "" for nothing */
	KindTable<Scroll, std::string> s_guess;
	KindTable<Potion, std::string> p_guess;
	KindTable<Ring, std::string> r_guess;
	KindTable<Stick, std::string> ws_guess;
	int group = 2;							/* Current group number */

	Items();
};

// The pool's creatures and items, as Lists find them (entities/List.hpp)
template <>
struct ListPool<Item> {
	static Item *at(ItemId id);
	static std::optional<ItemId> id_of(const Item &obj);
};
template <>
struct ListPool<Creature> {
	static Creature *at(CreatureId id);
	static std::optional<CreatureId> id_of(const Creature &tp);
};

/*
 * The creatures and items in play, made by new_creature() and new_item() and
 * given back by discard() (list.cpp). Each slot owns its thing; lists, packs
 * and the rest name them by Id (game/Id.hpp). The original allocated both kinds from one
 * array of MAXITEMS things (_things), so the count is shared: when it is
 * full, neither kind can be made, and level generation checks it.
 */
inline constexpr int MAXITEMS = 83;	/* things in the pool at most, both kinds */

struct Pool {
	Slots<Item, MAXITEMS> items;
	Slots<Creature, MAXITEMS> creatures;
	int total = 0;							/* Things of both kinds in use */

	// The thing a link names (see Id)
	Item &item(ItemId id) const { return items.get(id); }
	Creature &creature(CreatureId id) const { return creatures.get(id); }
	// The thing an optional link names, if it is in use
	Maybe<Item> item(std::optional<ItemId> id) const { return maybe(items.find(id)); }
	Maybe<Creature> creature(std::optional<CreatureId> id) const { return maybe(creatures.find(id)); }
	// The link to a thing, nullopt for none or a thing outside the pool
	std::optional<ItemId> id_of(const Item &obj) const { return items.id_of(&obj); }
	std::optional<CreatureId> id_of(const Creature &tp) const { return creatures.id_of(&tp); }
	std::optional<ItemId> id_of(Maybe<const Item> obj) const { return obj ? id_of(*obj) : std::nullopt; }
	std::optional<CreatureId> id_of(Maybe<const Creature> tp) const { return tp ? id_of(*tp) : std::nullopt; }
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
	// Where a destination is now (see Destination)
	Coord where(const Destination &dest) const;
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

// Shorthands for rng(): a number below range, the sum of number dice of
// sides, and nm give or take 10%
inline int rnd(int range) { return rng().below(range); }
inline int roll(int number, int sides) { return rng().roll(number, sides); }
inline int spread(int nm) { return rng().spread(nm); }

}  // namespace rogue
