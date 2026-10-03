#pragma once

/*
 * A fighting being: a monster or the rogue himself.
 *
 * Was the _t half of the legacy union thing.
 */

#include <optional>
#include <variant>

#include "core/Coord.hpp"
#include "core/Flags.hpp"
#include "entities/List.hpp"
#include "entities/Stats.hpp"
#include "game/Id.hpp"
#include "world/RoomRef.hpp"

namespace rogue {

/*
 * Where a running monster is headed (was a Coord * at the position): the
 * hero, a room's or passage's gold, or an item on the floor. Game::where()
 * gives the position, as it is when asked, as the pointer read it.
 */
struct Hero {
	friend constexpr bool operator==(Hero, Hero) = default;
};
struct Gold {
	RoomRef room;			/* the room or passage whose r_gold it is */

	friend constexpr bool operator==(const Gold &, const Gold &) = default;
};
using Destination = std::variant<Hero, Gold, ItemId>;

/* flags for creatures */
enum class CreatureFlag : unsigned short {
	Blind     = 0x0001,	/* ISBLIND: creature is blind */
	SeeMonst  = 0x0002,	/* SEEMONST: hero can detect unseen monsters */
	Running   = 0x0004,	/* ISRUN: creature is running at the player */
	Found     = 0x0008,	/* ISFOUND: creature has been seen (used for objects) */
	Invisible = 0x0010,	/* ISINVIS: creature is invisible */
	Mean      = 0x0020,	/* ISMEAN: creature can wake when player enters room */
	Greedy    = 0x0040,	/* ISGREED: creature runs to protect gold */
	Held      = 0x0080,	/* ISHELD: creature has been held */
	Confused  = 0x0100,	/* ISHUH: creature is confused */
	Regen     = 0x0200,	/* ISREGEN: creature can regenerate */
	CanConfuse = 0x0400,	/* CANHUH: creature can confuse */
	SeeInvisible = 0x0800,	/* CANSEE: creature can see invisible creatures */
	Cancelled = 0x1000,	/* ISCANC: creature has special qualities cancelled */
	Slow      = 0x2000,	/* ISSLOW: creature has been slowed */
	Hasted    = 0x4000,	/* ISHASTE: creature has been hastened */
	Flying    = 0x8000,	/* ISFLY: creature is of the flying type */
};
template <>
inline constexpr bool enable_flags<CreatureFlag> = true;
using CreatureFlags = Flags<CreatureFlag>;

struct Item;

struct Creature {
	Coord pos;				/* Position */
	char its_turn;				/* If slowed, is it a turn to move */
	char type;				/* What it is */
	unsigned char disguise;			/* What mimic looks like */
	unsigned char under;				/* Character that was where it was */
	std::optional<Destination> dest;	/* Where it is running to, if anywhere */
	CreatureFlags flags;		/* State word */
	entities::Stats stats;		/* Physical description */
	std::optional<RoomRef> room;	/* Current room for thing, if any */
	List<Item> pack;			/* What the thing is carrying */
};

}  // namespace rogue
