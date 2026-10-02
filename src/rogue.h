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
#include <span>
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
#include "core/Math.hpp"
#include "core/Maybe.hpp"
#include "core/Random.hpp"
#include "core/Text.hpp"
#include "entities/List.hpp"
#include "game/Slots.hpp"
#include "items/KindInfo.hpp"
#include "items/Kinds.hpp"
#include "rules/Experience.hpp"
#include "ui/Display.hpp"
#include "ui/Input.hpp"
#include "world/MapFlags.hpp"
#include "world/RoomRef.hpp"
#include "world/Trap.hpp"

#include "glyphs.h"
#include "platform/Clock.hpp"
#include "platform/Session.hpp"

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
 * Coordinate data type
 */
using rogue::Coord;  // see core/Coord.hpp
using rogue::Maybe;  // see core/Maybe.hpp
using rogue::maybe;
using rogue::refers_to;
using coord = rogue::Coord;

// Game output goes through the display, see ui/Display.hpp
using rogue::ui::display;
using rogue::ui::input;
using rogue::ui::TileStyle;

/*
 * Data type for strength values and modifiers
 */
typedef unsigned int str_t;

// The name, odds and worth of a kind of item, see items/KindInfo.hpp
using rogue::items::KindInfo;


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
using rogue::ItemId;
using rogue::CreatureId;
using rogue::Destination;
using rogue::Hero;
using rogue::Gold;
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


#include "game/Game.hpp"
#include "game/Pool.hpp"
#include "game/Keyboard.hpp"
#include "game/Messages.hpp"
#include "game/StatusLine.hpp"
#include "items/ItemCatalog.hpp"
#include "items/Identification.hpp"
#include "items/Inventory.hpp"
#include "items/effects/Potion.hpp"
#include "items/effects/Scroll.hpp"
#include "items/effects/Wand.hpp"
#include "items/effects/Ring.hpp"
#include "items/effects/Armor.hpp"
#include "items/effects/Weapon.hpp"
#include "rules/Combat.hpp"
#include "rules/Conditions.hpp"
#include "rules/Hunger.hpp"
#include "rules/Regeneration.hpp"
#include "rules/Strength.hpp"
#include "rules/Wandering.hpp"
#include "entities/MonsterCatalog.hpp"
#include "entities/MonsterAI.hpp"
#include "world/Rooms.hpp"
#include "world/Look.hpp"
#include "world/Traps.hpp"
#include "world/Maze.hpp"
#include "world/Passages.hpp"
#include "world/LevelGenerator.hpp"
#include "game/CommandDispatcher.hpp"
#include "game/Help.hpp"
#include "game/PlayerCommands.hpp"
#include "game/Movement.hpp"
#include "game/NewGame.hpp"
#include "game/GameLoop.hpp"
#include "game/Endings.hpp"
#include "persistence/SaveCommands.hpp"

using rogue::platform::fatal;
using rogue::platform::md_exit;
using rogue::readchar;
using rogue::flush_type;
using rogue::setup;
using rogue::credits;
using rogue::new_item;
using rogue::new_creature;
using rogue::discard;
using rogue::list_free;
using rogue::show_msg;
using rogue::add_msg;
using rogue::msg;
using rogue::addmsg;
using rogue::debug;
using rogue::ifterse;
using rogue::endmsg;
using rogue::more;
using rogue::putmsg;
using rogue::noterse;
using rogue::wait_for;
using rogue::wait_msg;
using rogue::str_attr;
using rogue::status;
using rogue::SIG2;
using rogue::items::w_names;
using rogue::items::a_names;
using rogue::items::a_chances;
using rogue::items::a_class;
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
using rogue::items::is_current;
using rogue::items::call_it;
using rogue::items::call;
using rogue::items::whatis;
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
using rogue::rules::e_levels;
using rogue::rules::he_man;
using rogue::rules::eat;
using rogue::rules::chg_str;
using rogue::rules::add_str;
using rogue::entities::MonsterKind;
using rogue::entities::monsters;
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
using rogue::entities::aggravate;
using rogue::world::roomin;
using rogue::world::diag_ok;
using rogue::world::cansee;
using rogue::world::rnd_pos;
using rogue::world::enter_room;
using rogue::world::leave_room;
using rogue::world::teleport;
using rogue::world::new_level;
using rogue::world::rnd_room;
using rogue::world::INDEX;
using rogue::world::offmap;
using rogue::world::winat;
using rogue::world::step_ok;
using rogue::world::find_obj;
using rogue::world::look;
using rogue::world::search;
using rogue::world::tr_name;
using rogue::world::be_trapped;
using rogue::world::descend;
using rogue::command;
using rogue::show_count;
using rogue::execcom;
using rogue::help;
using rogue::get_dir;
using rogue::find_dir;
using rogue::d_level;
using rogue::u_level;
using rogue::do_macro;
using rogue::do_run;
using rogue::do_move;
using rogue::rndmove;
using rogue::init_player;
using rogue::init_things;
using rogue::init_names;
using rogue::init_colors;
using rogue::init_stones;
using rogue::init_materials;
using rogue::getsyl;
using rogue::rchr;
using rogue::playit;
using rogue::quit;
using rogue::score;
using rogue::death;
using rogue::total_winner;
using rogue::killname;
using rogue::persistence::save_game;
using rogue::persistence::restore;

/*
 * Common strings
 * The state of a game is in game() (game/Game.hpp), and the fixed tables are
 * in the modules that use them.
 */

// The flash of a vorpal weapon: when it is made, and when it first sees its enemy
inline constexpr std::string_view intense = " of intense white light";
// a std::format string for msg()
inline constexpr std::string_view flashmsg = "your {} gives off a flash{}";

/*
 * Function types
 */

// legacy wrappers around rogue::rng()
inline int	rnd(int range) { return rogue::rng().below(range); }
inline int	roll(int number, int sides) { return rogue::rng().roll(number, sides); }
inline int	spread(int nm) { return rogue::rng().spread(nm); }
// The gold in a pile on this level (was GOLDCALC)
inline int	gold_calc() { return rnd(50 + 10 * game().level.depth) + 2; }

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

// Small helpers (core/Math.hpp, core/Text.hpp)
using rogue::sign;
using rogue::vowelstr;
using rogue::io_unctrl;

// ASCII character tests (core/Ascii.hpp)
using rogue::is_alpha;
using rogue::is_upper;
using rogue::is_lower;
using rogue::is_digit;
using rogue::is_space;
using rogue::is_print;
using rogue::to_upper;
using rogue::to_lower;

