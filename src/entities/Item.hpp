#pragma once

/*
 * An object: something lying on the floor or carried in a pack.
 *
 * Was the _o half of the legacy union thing. Included by rogue.h after the
 * legacy types it uses (coord, shint); game files include rogue.h.
 */

namespace rogue {

/* flags for objects */
enum class ItemFlag : unsigned short {
	Cursed   = 0x0001,	/* ISCURSED: object is cursed */
	Known    = 0x0002,	/* ISKNOW: player knows details about the object */
	DidFlash = 0x0004,	/* DIDFLASH: has the vorpal weapon flashed */
	Ego      = 0x0008,	/* ISEGO: weapon has control of player @ unused */
	/*@
	 * A scare monster scroll that was picked up once. The original set the
	 * creature flag ISFOUND on it, which is the same bit as ISEGO.
	 */
	Found    = 0x0008,
	Missile  = 0x0010,	/* ISMISL: object is a missile type */
	Many     = 0x0020,	/* ISMANY: object comes in groups */
	Revealed = 0x0040,	/* ISREVEAL: Do you know who the enemy of the object is */
};
template <>
inline constexpr bool enable_flags<ItemFlag> = true;
using ItemFlags = Flags<ItemFlag>;

struct Item {
	Item *l_next, *l_prev;		/* Next pointer in link */
	shint o_type;				/* What kind of object it is */
	coord o_pos;				/* Where it lives on the screen */
	char *o_text;				/* What it says if you read it */
	char o_launch;				/* What you need to launch it */
	const char *o_damage;		/* Damage if used like sword */
	const char *o_hurldmg;		/* Damage if thrown */
	shint o_count;				/* Count for plural objects */
	shint o_which;				/* Which object of a type it is */
	shint o_hplus;				/* Plusses to hit */
	shint o_dplus;				/* Plusses to damage */
	short o_ac;					/* Armor class (o_charges, o_goldval) */
	ItemFlags o_flags;			/* Information about objects */
	char o_enemy;				/* If it is enchanted, who it hates */
	shint o_group;				/* Group number for this object */
};

}  // namespace rogue
