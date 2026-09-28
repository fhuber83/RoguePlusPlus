/*
 * Rogue definitions and variable declarations
 *
 * rogue.h	1.4 (AI Design) 12/14/84
 */

#pragma once

#include <cerrno>
#include <clocale>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <format>
#include <optional>
#include <array>
#include <string>
#include <string_view>
#include <vector>

#include <unistd.h>

#include "core/Config.hpp"
#include "core/Ascii.hpp"
#include "core/Coord.hpp"
#include "core/Dice.hpp"
#include "core/Flags.hpp"
#include "core/KindTable.hpp"
#include "core/Random.hpp"
#include "entities/List.hpp"
#include "game/Slots.hpp"
#include "items/Kinds.hpp"
#include "ui/Display.hpp"
#include "ui/Input.hpp"
#include "world/MapFlags.hpp"
#include "world/RoomRef.hpp"
#include "world/Trap.hpp"

#include "glyphs.h"
#include "mach_dep.h"

/*
 * Screen size. Fixed at 80x25 (see rogue::ui::Screen); these used to be the
 * ncurses globals of the same name. Only game files see these; the curses
 * backend uses ncurses' own.
 */
inline constexpr int LINES = MAXLINES;
inline constexpr int COLS = MAXCOLS;
// Last line used for the map
inline constexpr int maxrow = MAXLINES - 2;


/*
 *  Options set for PC rogue
 */
inline constexpr int REV = 1;		/* the version, 1.48 */
inline constexpr int VER = 48;
inline constexpr std::string_view ENVFILE = "rogue.opt";

/*
 * Maximum number of different things
 */
inline constexpr int MAXROOMS = 9;
inline constexpr int MAXOBJ = 9;
inline constexpr int MAXPACK = 23;
inline constexpr int MAXTRAPS = 10;
inline constexpr int AMULETLEVEL = 26;
inline constexpr int NUMTHINGS = 7;	/* number of types of things */
inline constexpr int MAXPASS = 13;	/* upper limit on number of passages */
inline constexpr int MAXNAME = 20;	/* Maximum Length of a scroll */
inline constexpr int MAXITEMS = 83;	/* Maximum number of randomly generated things */
inline constexpr int BUFSIZE = 128;

/*
 * Various constants
 */
inline constexpr int MORETIME = 150;
inline constexpr int STOMACHSIZE = 2000;
inline constexpr int STARVETIME = 850;
inline constexpr int BOLT_LENGTH = 6;
inline constexpr int LAMPDIST = 3;

/*
 * Now we define the structures and types
 */

/*
 * Help list
 */
struct h_list {
	std::array<char, 5> h_chstr{};	// either (ch) or (ch,sep,ch2) appended with ": "
	std::size_t h_chlen = 0;
	std::string_view h_desc;

	// A line of text; an empty one ends the list (were H_STR and H_END)
	constexpr h_list(std::string_view desc) : h_desc(desc) {}
	// A glyph and what it is (was H_CHSTR)
	constexpr h_list(unsigned char ch, std::string_view desc)
		: h_chstr{static_cast<char>(ch), ':', ' '}, h_chlen(3), h_desc(desc) {}
	// Two glyphs with a separator, "A-Z" (was H_CH2STR)
	constexpr h_list(unsigned char first, unsigned char sep, unsigned char last, std::string_view desc)
		: h_chstr{static_cast<char>(first), static_cast<char>(sep), static_cast<char>(last), ':', ' '},
		  h_chlen(5), h_desc(desc) {}

	// The glyph column, "" for a line of text
	constexpr std::string_view glyphs() const { return {h_chstr.data(), h_chlen}; }
};

/*
 * Coordinate data type
 */
using rogue::Coord;  // see core/Coord.hpp
using coord = rogue::Coord;

// Game output goes through the display, see ui/Display.hpp
using rogue::ui::display;
using rogue::ui::input;
using rogue::ui::TileStyle;

/*
 * Data type for strength values and modifiers
 */
typedef unsigned int str_t;

/*
 * Stuff about magic items
 */

struct magic_item {
	std::string_view mi_name;
	int mi_prob;
	short mi_worth;
};


/*
 * Room structure
 */
namespace rogue {
enum class RoomFlag : unsigned short {
	Dark = 0x0001,	/* room is dark */
	Gone = 0x0002,	/* room is gone (a corridor) */
	Maze = 0x0004,	/* room is a maze */
};
template <>
inline constexpr bool enable_flags<RoomFlag> = true;
}  // namespace rogue
using rogue::RoomFlag;
using RoomFlags = rogue::Flags<RoomFlag>;

struct room {
	coord r_pos;			/* Upper left corner */
	coord r_max;			/* Size of room */
	coord r_gold;			/* Where the gold is */
	int r_goldval;			/* How much the gold is worth */
	RoomFlags r_flags;		/* Info about the room */
	int r_nexits;			/* Number of exits */
	coord r_exit[12];			/* Where the exits are */

	// A corridor where a room would be, but not a maze (was isgone())
	bool is_gone() const
	{
		return r_flags.test(RoomFlag::Gone) && !r_flags.test(RoomFlag::Maze);
	}
};

/*
 * Structure describing a fighting being
 */
struct stats {
	str_t s_str;			/* Strength */
	long s_exp;				/* Experience */
	int s_lvl;			/* Level of mastery */
	int s_arm;			/* Armor class */
	int s_hpt;			/* Hit points */
	rogue::Attacks s_dmg;		/* Damage done, per attack */
	int s_maxhp;			/* Max hit points */
};

/*
 * The legacy union thing is split into a creature (monster or player) and an
 * item. charges() and gold_value() are other names for o_ac.
 */
#include "entities/Item.hpp"
#include "entities/Creature.hpp"

using rogue::Creature;
using rogue::Item;
using rogue::List;
using rogue::ItemKind;
using rogue::KindTable;
using rogue::kind_count;
using rogue::kinds;
using rogue::Potion;
using rogue::Scroll;
using rogue::Ring;
using rogue::Stick;
using rogue::WeaponType;
using rogue::ArmorType;
using rogue::Food;
using rogue::Hand;
using rogue::Trap;
using rogue::MapFlag;
using rogue::MapFlags;
using rogue::RoomRef;
using rogue::ItemFilter;
using rogue::glyph_of;
using rogue::kind_of_glyph;
using rogue::CreatureFlags;
using rogue::ItemFlags;

/*
 * Various flag bits: rogue::ItemFlag and rogue::CreatureFlag, in Flags sets
 */
inline constexpr rogue::ItemFlag ISCURSED = rogue::ItemFlag::Cursed;
inline constexpr rogue::ItemFlag ISKNOW = rogue::ItemFlag::Known;
inline constexpr rogue::ItemFlag DIDFLASH = rogue::ItemFlag::DidFlash;
inline constexpr rogue::ItemFlag ISEGO = rogue::ItemFlag::Ego;
inline constexpr rogue::ItemFlag ISMISL = rogue::ItemFlag::Missile;
inline constexpr rogue::ItemFlag ISMANY = rogue::ItemFlag::Many;
inline constexpr rogue::ItemFlag ISREVEAL = rogue::ItemFlag::Revealed;
inline constexpr rogue::CreatureFlag ISBLIND = rogue::CreatureFlag::Blind;
inline constexpr rogue::CreatureFlag SEEMONST = rogue::CreatureFlag::SeeMonst;
inline constexpr rogue::CreatureFlag ISRUN = rogue::CreatureFlag::Running;
inline constexpr rogue::CreatureFlag ISFOUND = rogue::CreatureFlag::Found;
inline constexpr rogue::CreatureFlag ISINVIS = rogue::CreatureFlag::Invisible;
inline constexpr rogue::CreatureFlag ISMEAN = rogue::CreatureFlag::Mean;
inline constexpr rogue::CreatureFlag ISGREED = rogue::CreatureFlag::Greedy;
inline constexpr rogue::CreatureFlag ISHELD = rogue::CreatureFlag::Held;
inline constexpr rogue::CreatureFlag ISHUH = rogue::CreatureFlag::Confused;
inline constexpr rogue::CreatureFlag ISREGEN = rogue::CreatureFlag::Regen;
inline constexpr rogue::CreatureFlag CANHUH = rogue::CreatureFlag::CanConfuse;
inline constexpr rogue::CreatureFlag CANSEE = rogue::CreatureFlag::SeeInvisible;
inline constexpr rogue::CreatureFlag ISCANC = rogue::CreatureFlag::Cancelled;
inline constexpr rogue::CreatureFlag ISSLOW = rogue::CreatureFlag::Slow;
inline constexpr rogue::CreatureFlag ISHASTE = rogue::CreatureFlag::Hasted;
inline constexpr rogue::CreatureFlag ISFLY = rogue::CreatureFlag::Flying;


/*
 * Array containing information on all the various types of monsters
 */
struct monster {
	std::string_view m_name;		/* What to call the monster */
	int m_carry;			/* Probability of carrying something */
	CreatureFlags m_flags;		/* Things about the monster */
	struct stats m_stats;		/* Initial stats */
};

// The tables each game copies into game().items (extern.cpp)
extern const KindTable<Scroll, magic_item> s_magic_base;
extern const KindTable<Potion, magic_item> p_magic_base;
extern const KindTable<Ring, magic_item> r_magic_base;
extern const KindTable<Stick, magic_item> ws_magic_base;
extern const struct magic_item things_base[];

#include "game/Game.hpp"
#include "items/ItemCatalog.hpp"
#include "items/Identification.hpp"
#include "items/Inventory.hpp"
#include "items/effects/Potion.hpp"
#include "items/effects/Scroll.hpp"
#include "items/effects/Wand.hpp"
#include "items/effects/Ring.hpp"
#include "items/effects/Armor.hpp"
#include "items/effects/Weapon.hpp"
#include "rules/Daemons.hpp"
#include "rules/Combat.hpp"
#include "entities/MonsterCatalog.hpp"
#include "entities/MonsterAI.hpp"
#include "world/Rooms.hpp"
#include "world/Maze.hpp"
#include "world/Passages.hpp"
#include "world/LevelGenerator.hpp"
#include "game/CommandDispatcher.hpp"

using rogue::items::new_thing;
using rogue::items::inv_name;
using rogue::items::discovered;
using rogue::items::add_line;
using rogue::items::end_line;
using rogue::items::add_pack;
using rogue::items::pick_up;
using rogue::items::get_item;
using rogue::items::inventory;
using rogue::items::pack_char;
using rogue::items::money;
using rogue::items::drop;
using rogue::items::can_drop;
using rogue::items::effects::quaff;
using rogue::items::effects::invis_on;
using rogue::items::effects::turn_see;
using rogue::items::effects::th_effect;
using rogue::items::effects::read_scroll;
using rogue::items::effects::fix_stick;
using rogue::items::effects::do_zap;
using rogue::items::effects::drain;
using rogue::items::effects::fire_bolt;
using rogue::items::effects::charge_str;
using rogue::items::effects::ring_on;
using rogue::items::effects::ring_off;
using rogue::items::effects::ring_eat;
using rogue::items::effects::ring_num;
using rogue::items::effects::wear;
using rogue::items::effects::take_off;
using rogue::items::effects::waste_time;
using rogue::items::effects::missile;
using rogue::items::effects::do_motion;
using rogue::items::effects::fall;
using rogue::items::effects::init_weapon;
using rogue::items::effects::launched_by;
using rogue::items::effects::hit_monster;
using rogue::items::effects::num;
using rogue::items::effects::wield;
using rogue::items::effects::tick_pause;
using rogue::rules::Event;
using rogue::rules::start_daemon;
using rogue::rules::fuse;
using rogue::rules::lengthen;
using rogue::rules::extinguish;
using rogue::rules::do_daemons;
using rogue::rules::do_fuses;
using rogue::rules::doctor;
using rogue::rules::swander;
using rogue::rules::rollwand;
using rogue::rules::unconfuse;
using rogue::rules::unsee;
using rogue::rules::sight;
using rogue::rules::nohaste;
using rogue::rules::stomach;
using rogue::rules::fight;
using rogue::rules::attack;
using rogue::rules::swing;
using rogue::rules::check_level;
using rogue::rules::save_throw;
using rogue::rules::SaveThrow;
using rogue::rules::save;
using rogue::rules::is_magic;
using rogue::rules::raise_level;
using rogue::rules::killed;
using rogue::entities::randmonster;
using rogue::entities::pick_mons;
using rogue::entities::new_monster;
using rogue::entities::f_restor;
using rogue::entities::flytrap_attacks;
using rogue::entities::wanderer;
using rogue::entities::give_pack;
using rogue::entities::wake_monster;
using rogue::entities::moat;
using rogue::entities::runners;
using rogue::entities::start_run;
using rogue::entities::see_monst;
using rogue::entities::find_dest;
using rogue::entities::slime_split;
using rogue::entities::plop_monster;
using rogue::world::roomin;
using rogue::world::diag_ok;
using rogue::world::cansee;
using rogue::world::rnd_pos;
using rogue::world::enter_room;
using rogue::world::leave_room;
using rogue::world::new_level;
using rogue::world::rnd_room;
using rogue::command;
using rogue::show_count;
using rogue::execcom;

/*
 * External variables
 * The state of a game is in game() (game/Game.hpp). What is left here are
 * fixed tables and strings (extern.cpp, init.cpp).
 */

// The ranks, by experience level (he_man[level - 1])
extern const std::array<std::string_view, 21> he_man;
inline constexpr std::string_view intense = " of intense white light";
// Weapon names, and the name of the WeaponType::Flame that fire_bolt() throws
extern KindTable<WeaponType, std::string_view, kind_count<WeaponType> + 1> w_names;
extern const KindTable<ArmorType, std::string_view> a_names;
// a std::format string for msg()
inline constexpr std::string_view flashmsg = "your {} gives off a flash{}";
extern const struct h_list helpcoms[], helpobjs[];
extern const KindTable<ArmorType, int> a_chances, a_class;
extern const struct monster monsters[];

// the experience level table (init.cpp)
extern const long e_levels[20];

/*
 * Function types
 * mach_dep.cpp functions are declared in mach_dep.h
 */

// init.cpp
void	init_player(void);
void	init_things(void);
void	init_colors(void);
void	init_names(void);
void	init_stones(void);
void	init_materials(void);
char	*getsyl(void);
char	rchr(std::string_view string);

// io.cpp
// msg(), addmsg() and ifterse() take std::format strings. An empty msg() clears the line.
void	show_msg(std::string_view text);
void	add_msg(std::string_view text);

template <class... Args>
void
msg(std::format_string<Args...> fmt, Args &&...args)
{
	show_msg(std::format(fmt, std::forward<Args>(args)...));
}

template <class... Args>
void
addmsg(std::format_string<Args...> fmt, Args &&...args)
{
	add_msg(std::format(fmt, std::forward<Args>(args)...));
}

// A message from the consistency checks (rogue::config::debug_checks)
template <class... Args>
void
debug(std::format_string<Args...> fmt, Args &&...args)
{
	show_msg(std::format(fmt, std::forward<Args>(args)...));
}

template <class... Args>
void
ifterse(std::format_string<Args...> tfmt, std::format_string<Args...> fmt, Args &&...args)
{
	msg(game().options.expert ? tfmt : fmt, std::forward<Args>(args)...);
}

void	wait_msg(std::string_view msg);
void	endmsg(void);
void	more(std::string_view msg);
void	putmsg(std::string_view msg);
void	status(void);
void	wait_for(unsigned char ch);
void	str_attr(std::string_view str);
void	SIG2(void);
std::string	io_unctrl(unsigned char ch);
std::string_view	noterse(std::string_view str);

// list.cpp
Item	*new_item(void);
Creature	*new_creature(void);
int	discard(Item *item);
int	discard(Creature *item);

/*
 * Empties a list of creatures or items and gives them back to the pool
 */
template <class T>
void
list_free(rogue::List<T> &list)
{
	T *item;

	while ((item = list.first()) != nullptr)
	{
	list.remove(item);
	discard(item);
	}
}

// playit.cpp
void	endit(void);
void	playit(const std::optional<std::string> &sname);
void	quit(void);
void	leave(void);
// legacy wrappers around rogue::rng()
inline int	rnd(int range) { return rogue::rng().below(range); }
inline int	roll(int number, int sides) { return rogue::rng().roll(number, sides); }
// The gold in a pile on this level (was GOLDCALC)
inline int	gold_calc() { return rnd(50 + 10 * game().level.depth) + 2; }

// misc.cpp
void	look(bool wakeup);
void	eat(void);
void	chg_str(int amt);
void	add_str(str_t *sp, int amt);
void	aggravate(void);
void	call_it(bool know, std::string &guess);
void	help(const struct h_list *helpscr);
void	search(void);
void	d_level(void);
void	u_level(void);
void	call(void);
void	do_macro(std::string &macro);
Item	*find_obj(int y, int x);
bool	add_haste(bool potion);
bool	is_current(Item *obj);
bool	get_dir(void);
std::optional<Coord>	find_dir(unsigned char ch);
bool	step_ok(unsigned char ch);
bool	offmap(int y, int x);
std::string_view	tr_name(Trap type);
std::string_view	vowelstr(std::string_view str);
char	goodch(Item *obj);
int	sign(int nm);
unsigned char	winat(int y, int x);
int	spread(int nm);
/*
 * How long things last, each spread by 10% (were BEARTIME, SLEEPTIME, ...)
 */
inline int	bear_time() { return spread(3); }		/* held by a bear trap */
inline int	sleep_time() { return spread(5); }		/* asleep from a gas trap or scroll */
inline int	hold_time() { return spread(2); }		/* paralyzed by a potion */
inline int	wander_time() { return spread(70); }	/* until the next wandering monster */
inline int	huh_duration() { return spread(20); }	/* confused */
inline int	see_duration() { return spread(300); }	/* seeing invisible, or blind */
inline int	hunger_time() { return spread(1300); }	/* a full stomach */
int	DISTANCE(int y1, int x1, int y2, int x2);
int	INDEX(int y, int x);

// move.cpp
void	do_run(unsigned char ch);
void	do_move(int dy, int dx);
void	door_open(struct room *rp);
void	descend(std::string_view mesg);
Coord	rndmove(Creature *who);

// rip.cpp
void	score(int amount, int flags, char monst);
void	death(char monst);
void	total_winner(void);
std::string	killname(unsigned char monst, bool doart);

// save.cpp
void	save_game(void);
void	restore(const std::string &savefile);

// ASCII character tests (core/Ascii.hpp)
using rogue::is_alpha;
using rogue::is_upper;
using rogue::is_lower;
using rogue::is_digit;
using rogue::is_space;
using rogue::is_print;
using rogue::to_upper;
using rogue::to_lower;

// wizard.cpp
void	whatis(void);
int	teleport(void);
