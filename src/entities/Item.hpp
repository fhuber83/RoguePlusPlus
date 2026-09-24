#pragma once

/*
 * An object: something lying on the floor or carried in a pack.
 *
 * Was the _o half of the legacy union thing. Included by rogue.h after the
 * legacy types it uses (coord, shint) and the glyph codes; game files include
 * rogue.h.
 */

#include <optional>

namespace rogue {

/*
 * What kind of object it is. Used to be its glyph (POTION == 0xad), which
 * the map shows; glyph_of() and kind_of_glyph() convert now.
 */
enum class ItemKind : unsigned char {
	None,		/* not made yet */
	Potion,
	Scroll,
	Food,
	Weapon,
	Armor,
	Ring,
	Stick,
	Amulet,
	Gold,
	Missile,	/* the magic missile a wand shoots ('*'), never picked up */
};

constexpr byte
glyph_of(ItemKind kind)
{
	switch (kind) {
	case ItemKind::Potion:	return POTION;
	case ItemKind::Scroll:	return SCROLL;
	case ItemKind::Food:	return FOOD;
	case ItemKind::Weapon:	return WEAPON;
	case ItemKind::Armor:	return ARMOR;
	case ItemKind::Ring:	return RING;
	case ItemKind::Stick:	return STICK;
	case ItemKind::Amulet:	return AMULET;
	case ItemKind::Gold:	return GOLD;
	case ItemKind::Missile:	return '*';
	case ItemKind::None:	break;
	}
	return ' ';
}

// The kind of item a map glyph shows, if it shows one
constexpr std::optional<ItemKind>
kind_of_glyph(byte glyph)
{
	switch (glyph) {
	case POTION:	return ItemKind::Potion;
	case SCROLL:	return ItemKind::Scroll;
	case FOOD:		return ItemKind::Food;
	case WEAPON:	return ItemKind::Weapon;
	case ARMOR:		return ItemKind::Armor;
	case RING:		return ItemKind::Ring;
	case STICK:		return ItemKind::Stick;
	case AMULET:	return ItemKind::Amulet;
	case GOLD:		return ItemKind::Gold;
	}
	return std::nullopt;
}

/*
 * Which items of the pack get_item() and inventory() offer: those of one
 * kind, all of them, or those that can be named with 'c' (were the type
 * values 0 and CALLABLE).
 */
class ItemFilter {
public:
	constexpr ItemFilter(ItemKind kind) : match_(Match::Kind), kind_(kind) {}
	static constexpr ItemFilter all() { return ItemFilter(Match::All); }
	static constexpr ItemFilter callable() { return ItemFilter(Match::Callable); }

	constexpr bool is(ItemKind kind) const { return match_ == Match::Kind && kind_ == kind; }
	constexpr bool is_all() const { return match_ == Match::All; }
	constexpr bool is_callable() const { return match_ == Match::Callable; }

private:
	enum class Match : unsigned char { Kind, All, Callable };

	constexpr explicit ItemFilter(Match match) : match_(match), kind_(ItemKind::None) {}

	Match match_;
	ItemKind kind_;
};

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
	ItemKind o_type;			/* What kind of object it is */
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
