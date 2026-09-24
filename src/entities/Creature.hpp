#pragma once

/*
 * A fighting being: a monster or the rogue himself.
 *
 * Was the _t half of the legacy union thing. Included by rogue.h after the
 * legacy types it uses (coord, struct stats, struct room); game files include
 * rogue.h.
 */

namespace rogue {

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
	coord t_pos;				/* Position */
	char t_turn;				/* If slowed, is it a turn to move */
	char t_type;				/* What it is */
	byte t_disguise;			/* What mimic looks like */
	byte t_oldch;				/* Character that was where it was */
	coord *t_dest;				/* Where it is running to */
	CreatureFlags t_flags;		/* State word */
	struct stats t_stats;		/* Physical description */
	struct room *t_room;		/* Current room for thing */
	List<Item> t_pack;			/* What the thing is carrying */
};

}  // namespace rogue
