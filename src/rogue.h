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
#include "entities/Stats.hpp"
#include "game/Slots.hpp"
#include "items/KindInfo.hpp"
#include "items/Kinds.hpp"
#include "rules/Experience.hpp"
#include "ui/Display.hpp"
#include "ui/Input.hpp"
#include "world/MapFlags.hpp"
#include "world/Room.hpp"
#include "world/RoomRef.hpp"
#include "world/Trap.hpp"

#include "core/Glyphs.hpp"
#include "platform/Clock.hpp"
#include "platform/Session.hpp"

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

// The name, odds and worth of a kind of item, see items/KindInfo.hpp
using rogue::items::KindInfo;


// A room or passage (world/Room.hpp), and a fighting being's stats
// (entities/Stats.hpp)
using rogue::RoomFlag;
using rogue::RoomFlags;
using rogue::world::Room;
using rogue::entities::Stats;
using rogue::entities::str_t;

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

#include "rules/Durations.hpp"

// Moved to their modules in phase 14.1; the files that still include rogue.h
// see them here until 14.3
using rogue::MAXSTR;
using rogue::MAXLINES;
using rogue::MAXCOLS;
using rogue::maxrow;
using rogue::ctrl;
using rogue::ESCAPE;
using rogue::is_floor;
using rogue::is_monster;
using rogue::BUFSIZE;
using rogue::MAXNAME;
using rogue::MAXITEMS;
using rogue::rnd;
using rogue::roll;
using rogue::spread;
using rogue::items::NUMTHINGS;
using rogue::items::MAXPACK;
using rogue::items::effects::BOLT_LENGTH;
using rogue::items::effects::intense;
using rogue::items::effects::flashmsg;
using rogue::world::MAXROOMS;
using rogue::world::MAXPASS;
using rogue::world::AMULETLEVEL;
using rogue::world::LAMPDIST;
using rogue::world::gold_calc;
using rogue::rules::bear_time;
using rogue::rules::sleep_time;
using rogue::rules::hold_time;
using rogue::rules::wander_time;
using rogue::rules::huh_duration;
using rogue::rules::see_duration;
using rogue::rules::hunger_time;
using rogue::ItemFlag;
using rogue::CreatureFlag;

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

