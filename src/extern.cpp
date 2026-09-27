/*
 * global variable initializaton
 *
 * @(#)extern.c	5.2 (Berkeley) 6/16/82
 */

#include "rogue.h"

/*
 * All this should be low as possible in memory so that
 * we can save the min
 */
KindTable<WeaponType, std::string_view, kind_count<WeaponType> + 1> w_names = {	/* Names of the various weapons */
	"mace",
	"long sword",
	"short bow",
	"arrow",
	"dagger",
	"two handed sword",
	"dart",
	"crossbow",
	"crossbow bolt",
	"spear",
	""					/* fake entry for dragon's breath, set by fire_bolt() */
};
constexpr KindTable<ArmorType, std::string_view> a_names = {		/* Names of armor types */
	"leather armor",
	"ring mail",
	"studded leather armor",
	"scale mail",
	"chain mail",
	"splint mail",
	"banded mail",
	"plate mail"
};

const KindTable<ArmorType, int> a_chances = {		/* Chance for each armor type */
	20,
	35,
	50,
	63,
	75,
	85,
	95,
	100
};
const KindTable<ArmorType, int> a_class = {		/* Armor class for each armor type */
	8,
	7,
	7,
	6,
	5,
	4,
	4,
	3
};

/*
 * The odds and worth of each kind of item. Each game works on a copy in
 * game().items, since init_*() accumulate the odds and add the stone value
 * to the worth of rings.
 */
const KindTable<Scroll, magic_item> s_magic_base = {
	{ "monster confusion",	 8, 140 },
	{ "magic mapping",		 5, 150 },
	{ "hold monster",		 3, 180 },
	{ "sleep",			 5,   5 },
	{ "enchant armor",		 8, 160 },
	{ "identify",		27, 100 },
	{ "scare monster",		 4, 200 },
	{ "food detection",		 4,  50 },
	{ "teleportation",		 7, 165 },
	{ "enchant weapon",		10, 150 },
	{ "create monster",		 5,  75 },
	{ "remove curse",		 8, 105 },
	{ "aggravate monsters",	 4,  20 },
	{ "blank paper",		 1,   5 },
	{ "vorpalize weapon",	 1, 300 }
};

const KindTable<Potion, magic_item> p_magic_base = {
	{ "confusion",		 8,   5 },
	{ "paralysis",		10,   5 },
	{ "poison",			 8,   5 },
	{ "gain strength",		15, 150 },
	{ "see invisible",		 2, 100 },
	{ "healing",		15, 130 },
	{ "monster detection",	 6, 130 },
	{ "magic detection",	 6, 105 },
	{ "raise level",		 2, 250 },
	{ "extra healing",		 5, 200 },
	{ "haste self",		 4, 190 },
	{ "restore strength",	14, 130 },
	{ "blindness",		 4,   5 },
	{ "thirst quenching",	 1,   5 }
};

const KindTable<Ring, magic_item> r_magic_base = {
	{ "protection",		 9, 400 },
	{ "add strength",		 9, 400 },
	{ "sustain strength",	 5, 280 },
	{ "searching",		10, 420 },
	{ "see invisible",		10, 310 },
	{ "adornment",		 1,  10 },
	{ "aggravate monster",	10,  10 },
	{ "dexterity",		 8, 440 },
	{ "increase damage",	 8, 400 },
	{ "regeneration",		 4, 460 },
	{ "slow digestion",		 9, 240 },
	{ "teleportation",		 5,  30 },
	{ "stealth",		 7, 470 },
	{ "maintain armor",		 5, 380 }
};

const KindTable<Stick, magic_item> ws_magic_base = {
	{ "light",			12, 250 },
	{ "striking",		 9,  75 },
	{ "lightning",		 3, 330 },
	{ "fire",			 3, 330 },
	{ "cold",			 3, 330 },
	{ "polymorph",		15, 310 },
	{ "magic missile",		10, 170 },
	{ "haste monster",		 9,   5 },
	{ "slow monster",		11, 350 },
	{ "drain life",		 9, 300 },
	{ "nothing",		 1,   5 },
	{ "teleport away",		 5, 340 },
	{ "teleport to",		 5,  50 },
	{ "cancellation",		 5, 280 }
};

/*
 * Original code used CP437 codes hard coded inside the help strings,
 * instead of the #define'd char constants for FLOOR, PLAYER etc.
 * To support the constants, helpcoms/helpobjs array type has changed from
 * string to struct h_list, whose constructors build the glyph column.
 *
 * Ironically, struct h_list already existed in rogue.h, but it was unused in
 * code, so perhaps original authors either abandoned the idea or were halfway
 * through implementing it.
 */
const struct h_list helpcoms[] = {
	{"F1     list of commands"},
	{"F2     list of symbols"},
	{"F3     repeat command"},
	{"F4     repeat message"},
	{"F5     rename something"},
	{"F6     recall what's been discovered"},
	{"F7     inventory of your possessions"},
	{"F8     <dir> identify trap type"},
	{"F9     The Any Key (definable)"},
	{"Alt F9 defines the Any Key"},
	{"Space  Clear -More- message"},
	{"\x11\xd9     the Enter Key"},
	{"\x1b      left"},
	{"\x19      down"},
	{"\x18      up"},
	{"\x1a      right"},
	{"Home   up & left"},
	{"PgUp   up & right"},
	{"End    down & left"},
	{"PgDn   down & right"},
	{"Scroll Fast Play mode"},
	{".      rest"},
	{">      go down a staircase"},
	{"<      go up a staircase"},
	{"Esc    cancel command"},
	{"d      drop object"},
	{"e      eat food"},
	{"f      <dir> find something"},
	{"q      quaff potion"},
	{"r      read paper"},
	{"s      search for trap/secret door"},
	{"t      <dir> throw something"},
	{"w      wield a weapon"},
	{"z      <dir> zap with a wand"},
	{"B      run down & left"},
	{"H      run left"},
	{"J      run down"},
	{"K      run up"},
	{"L      run right"},
	{"N      run down & right"},
	{"U      run up & right"},
	{"Y      run up & left"},
	{"W      wear armor"},
	{"T      take armor off"},
	{"P      put on ring"},
	{"Q      quit"},
	{"R      remove ring"},
	{"S      save game"},
	{"^      identify trap"},
	{"?      help"},
	{"/      key"},
	{"+      throw"},
	{"-      zap"},
	{"Ctrl t terse message format"},
	{"Ctrl r repeat message"},
	{"Del    search for something hidden"},
	{"Ins    <dir> find something"},
	{"a      repeat command"},
	{"c      rename something"},
	{"i      inventory"},
	{"v      version number"},
	{"D      list what has been discovered"},
	{""}		/* the end */
};

const struct h_list helpobjs[] = {
	{FLOOR,   "the floor"},
	{PLAYER,  "the hero"},
	{FOOD,    "some food"},
	{AMULET,  "the amulet of yendor"},
	{SCROLL,  "a scroll"},
	{WEAPON,  "a weapon"},
	{ARMOR,   "a piece of armor"},
	{GOLD,    "some gold"},
	{STICK,   "a magic staff"},
	{POTION,  "a potion"},
	{RING,    "a magic ring"},
	{0xB2,    "a passage"},  // not PASSAGE (0xB1)
	/* make sure in 40 or 80 column none of line draw set connects */
	/* this is currently in column 1 for 80 */
	{DOOR,    "a door"},
	{ULWALL,  "an upper left corner"},
	{TRAP,    "a trap"},
	{HWALL,   "a horizontal wall"},
	{LRWALL,  "a lower right corner"},
	{LLWALL,  "a lower left corner"},
	{VWALL,   "a vertical wall"},
	{URWALL,  "an upper right corner"},
	{STAIRS,  "a stair case"},
	{MAGIC, ',', BMAGIC, "safe and perilous magic"},
	{'A', '-', 'Z', "26 different monsters"},
	{""}		/* the end */
};
/*
 * Names of the various experience levels
 */

constexpr std::array<std::string_view, 21> he_man = std::to_array<std::string_view>({
	"",
	"Guild Novice",
	"Apprentice",
	"Journeyman",
	"Adventurer",
	"Fighter",
	"Warrior",
	"Rogue",
	"Champion",
	"Master Rogue",
	"Warlord",
	"Hero",
	"Guild Master",
	"Dragonlord",
	"Wizard",
	"Rogue Geek",
	"Rogue Addict",
	"Schmendrick",
	"Gunfighter",
	"Time Waster",
	"Bug Chaser"
});

/* bool askme = true; */			/* Ask about unidentified things */
/* bool fight_flush = true;	*/	/* True if toilet input */
/* bool jump = false;	*/		/* Show running as series of jumps */
/* bool passgo = true;	*/		/* Follow passages */
/* bool slow_invent = false; */		/* Inventory one line at a time */
/* char *release;	*/			/* Release number of rogue */
/* WINDOW *hw;				 Used as a scratch window */

/*
 * A value the game never reads (was ___): s_hpt and s_maxhp of the monster
 * templates, as each new monster rolls its hit points, and the worth of the
 * kinds of item.
 */
constexpr int NA = 1;
// Every monster's strength
constexpr str_t XX = 10;

const struct monster monsters[26] =
{
	/* Name		 CARRY	FLAG    str, exp, lvl, amr, hpt, dmg, maxhp */
	{ "aquator",	0,	ISMEAN,	{ XX, 20,   5,   2, NA, "0d0/0d0", NA } },
	{ "bat",	 	0,	ISFLY,	{ XX,  1,   1,   3, NA, "1d2", NA } },
	{ "centaur",	 15,	{},	{ XX, 25,   4,   4, NA, "1d6/1d6", NA } },
	{ "dragon",	 100,	ISMEAN,	{ XX,6800, 10,  -1, NA, "1d8/1d8/3d10", NA } },
	{ "emu",	 0,	ISMEAN,	{ XX,  2,   1,   7, NA, "1d2", NA } },
		/* until one hits; then every flytrap does fung_hit d1, see flytrap_attacks() */
		/* string with others, since it is written on in the program */
	{ "venus flytrap",0,	ISMEAN,	{ XX, 80,   8,   3, NA, "0d0", NA } },
	{ "griffin",	 20,	ISMEAN|ISFLY|ISREGEN,	{XX,2000, 13, 2,NA, "4d3/3d5/4d3", NA } },
	{ "hobgoblin",	 0,	ISMEAN,	{ XX,  3,   1,   5, NA, "1d8", NA } },
	{ "ice monster", 0,	ISMEAN,	{ XX,  15,   1,   9, NA, "1d2", NA } },
	{ "jabberwock",  70,	{},	{ XX,4000, 15,   6, NA, "2d12/2d4", NA } },
	{ "kestral",	 0,	ISMEAN|ISFLY, { XX,  1,   1,   7, NA, "1d4", NA } },
		/*
		 * The original has ISGREED (0x40) in the CARRY column: leprechauns
		 * carry something 64% of the time and are not greedy. Kept as is.
		 */
	{ "leprechaun",	 0x40,	{},	{ XX, 10,   3,   8, NA, "1d2", NA } },
	{ "medusa",	 40,	ISMEAN,	{ XX,200,   8,   2, NA, "3d4/3d4/2d5", NA } },
	{ "nymph",	 100,	{},	{ XX, 37,   3,   9, NA, "0d0", NA } },
	{ "orc",	 15,	ISGREED,{ XX,  5,   1,   6, NA, "1d8", NA } },
	{ "phantom",	 0,ISINVIS,{ XX,120,   8,   3, NA, "4d4", NA } },
	{ "quagga",	 30,	ISMEAN,	{ XX, 32,   3,   2, NA, "1d2/1d2/1d4", NA } },
	{ "rattlesnake", 0,	ISMEAN,	{ XX,  9,   2,   3, NA, "1d6", NA } },
	{ "slime",	 	 0,	ISMEAN,	{ XX,  1,   2,   8, NA, "1d3", NA } },
	{ "troll",	 50,	ISREGEN|ISMEAN,{ XX, 120, 6, 4, NA, "1d8/1d8/2d6", NA } },
	{ "ur-vile",	 0,	ISMEAN,	{ XX,190,   7,  -2, NA, "1d3/1d3/1d3/4d6", NA } },
	{ "vampire",	 20,	ISREGEN|ISMEAN,{ XX,350,   8,   1, NA, "1d10", NA } },
	{ "wraith",	 0,	{},	{ XX, 55,   5,   4, NA, "1d6", NA } },
	{ "xeroc",30,	{},	{ XX,100,   7,   7, NA, "3d4", NA } },
	{ "yeti",	 30,	{},	{ XX, 50,   4,   6, NA, "1d6/1d6", NA } },
	{ "zombie",	 0,	ISMEAN,	{ XX,  6,   2,   8, NA, "1d8", NA } }
};

/*
 * The odds of each kind of random item. init_things() accumulates them in
 * the game's copy, and the only user is new_thing(). mi_worth is unused (NA).
 */
const struct magic_item things_base[NUMTHINGS] = {
	{ "",			27, NA },	/* potion */
	{ "",			30, NA },	/* scroll */
	{ "",			17, NA },	/* food */
	{ "",			 8, NA },	/* weapon */
	{ "",			 8, NA },	/* armor */
	{ "",			 5, NA },	/* ring */
	{ "",			 5, NA }	/* stick */
};

