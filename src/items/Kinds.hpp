#pragma once

#include "core/KindTable.hpp"

/*
 * Which potion, scroll, ring, ... an item is: its number, read with
 * Item::which<E>(). The numbers are the original's (the old P_*, S_*, R_*,
 * WS_*, weapon and armor defines, named in the comments), and the order of
 * the tables they index.
 */

namespace rogue {

enum class Potion {
	Confusion,			/* P_CONFUSE */
	Paralysis,			/* P_PARALYZE */
	Poison,				/* P_POISON */
	GainStrength,		/* P_STRENGTH */
	SeeInvisible,		/* P_SEEINVIS */
	Healing,			/* P_HEALING */
	MonsterDetection,	/* P_MFIND */
	MagicDetection,		/* P_TFIND */
	RaiseLevel,			/* P_RAISE */
	ExtraHealing,		/* P_XHEAL */
	Haste,				/* P_HASTE */
	RestoreStrength,	/* P_RESTORE */
	Blindness,			/* P_BLIND */
	ThirstQuenching,	/* P_NOP */
};
template <>
inline constexpr std::size_t kind_count<Potion> = 14;

enum class Scroll {
	MonsterConfusion,	/* S_CONFUSE */
	MagicMapping,		/* S_MAP */
	HoldMonster,		/* S_HOLD */
	Sleep,				/* S_SLEEP */
	EnchantArmor,		/* S_ARMOR */
	Identify,			/* S_IDENT */
	ScareMonster,		/* S_SCARE */
	FoodDetection,		/* S_GFIND */
	Teleportation,		/* S_TELEP */
	EnchantWeapon,		/* S_ENCH */
	CreateMonster,		/* S_CREATE */
	RemoveCurse,		/* S_REMOVE */
	AggravateMonsters,	/* S_AGGR */
	BlankPaper,			/* S_NOP */
	Vorpalize,			/* S_VORPAL */
};
template <>
inline constexpr std::size_t kind_count<Scroll> = 15;

enum class Ring {
	Protection,			/* R_PROTECT */
	AddStrength,		/* R_ADDSTR */
	SustainStrength,	/* R_SUSTSTR */
	Searching,			/* R_SEARCH */
	SeeInvisible,		/* R_SEEINVIS */
	Adornment,			/* R_NOP */
	AggravateMonster,	/* R_AGGR */
	Dexterity,			/* R_ADDHIT */
	IncreaseDamage,		/* R_ADDDAM */
	Regeneration,		/* R_REGEN */
	SlowDigestion,		/* R_DIGEST */
	Teleportation,		/* R_TELEPORT */
	Stealth,			/* R_STEALTH */
	MaintainArmor,		/* R_SUSTARM */
};
template <>
inline constexpr std::size_t kind_count<Ring> = 14;

// The hand a ring is worn on (was LEFT and RIGHT)
enum class Hand {
	Left,
	Right,
};
template <>
inline constexpr std::size_t kind_count<Hand> = 2;

// Wands and staffs
enum class Stick {
	Light,				/* WS_LIGHT */
	Striking,			/* WS_HIT */
	Lightning,			/* WS_ELECT */
	Fire,				/* WS_FIRE */
	Cold,				/* WS_COLD */
	Polymorph,			/* WS_POLYMORPH */
	MagicMissile,		/* WS_MISSILE */
	HasteMonster,		/* WS_HASTE_M */
	SlowMonster,		/* WS_SLOW_M */
	DrainLife,			/* WS_DRAIN */
	Nothing,			/* WS_NOP */
	TeleportAway,		/* WS_TELAWAY */
	TeleportTo,			/* WS_TELTO */
	Cancellation,		/* WS_CANCEL */
	/*
	 * Not a stick: a vorpalized weapon zapped at its enemy, which do_zap()
	 * handles like one (was MAXSTICKS)
	 */
	Vorpal,
};
template <>
inline constexpr std::size_t kind_count<Stick> = 14;

enum class WeaponType {
	Mace,				/* MACE */
	LongSword,			/* SWORD */
	ShortBow,			/* BOW */
	Arrow,				/* ARROW */
	Dagger,				/* DAGGER */
	TwoHandedSword,		/* TWOSWORD */
	Dart,				/* DART */
	Crossbow,			/* CROSSBOW */
	CrossbowBolt,		/* BOLT */
	Spear,				/* SPEAR */
	/*
	 * Not a weapon: the dragon's breath, which fire_bolt() throws like one
	 * (FLAME). It has an entry in w_names only.
	 */
	Flame,
};
template <>
inline constexpr std::size_t kind_count<WeaponType> = 10;

enum class ArmorType {
	Leather,			/* LEATHER */
	RingMail,			/* RING_MAIL */
	StuddedLeather,		/* STUDDED_LEATHER */
	ScaleMail,			/* SCALE_MAIL */
	ChainMail,			/* CHAIN_MAIL */
	SplintMail,			/* SPLINT_MAIL */
	BandedMail,			/* BANDED_MAIL */
	PlateMail,			/* PLATE_MAIL */
};
template <>
inline constexpr std::size_t kind_count<ArmorType> = 8;

// Was the number 0 or 1
enum class Food {
	Ration,
	Fruit,				/* the fruit named by the fruit option */
};
template <>
inline constexpr std::size_t kind_count<Food> = 2;

}  // namespace rogue
