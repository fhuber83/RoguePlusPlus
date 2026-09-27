/*
 * global variable initializaton
 *
 * init.c	1.4 (A.I. Design) 12/14/84
 */

#include "rogue.h"


/*
 * init_player:
 *	Roll up the rogue
 */
void
init_player()
{
	Item *obj;
	game().player.body.t_stats = game().player.max_stats;
	game().player.food_left = hunger_time();
	/*
	 * initialize things
	 */
	game().pool = rogue::Pool();
	/*
	 * Give the rogue his weaponry.  First a mace.
	 */
	obj = new_item();
	obj->o_type = ItemKind::Weapon;
	obj->set_which(WeaponType::Mace);
	init_weapon(obj, WeaponType::Mace);
	obj->o_hplus = 1;
	obj->o_dplus = 1;
	obj->o_flags.set(ISKNOW);
	obj->o_count = 1;
	obj->o_group = 0;
	add_pack(obj, true);
	game().player.weapon = obj;
	/*
	 * Now a +1 bow
	 */
	obj = new_item();
	obj->o_type = ItemKind::Weapon;
	obj->set_which(WeaponType::ShortBow);
	init_weapon(obj, WeaponType::ShortBow);
	obj->o_hplus = 1;
	obj->o_dplus = 0;
	obj->o_count = 1;
	obj->o_group = 0;
	obj->o_flags.set(ISKNOW);
	add_pack(obj, true);
	/*
	 * Now some arrows
	 */
	obj = new_item();
	obj->o_type = ItemKind::Weapon;
	obj->set_which(WeaponType::Arrow);
	init_weapon(obj, WeaponType::Arrow);
	obj->o_count = rnd(15) + 25;
	obj->o_hplus = obj->o_dplus = 0;
	obj->o_flags.set(ISKNOW);
	add_pack(obj, true);
	/*
	 * And his suit of armor
	 */
	obj = new_item();
	obj->o_type = ItemKind::Armor;
	obj->set_which(ArmorType::RingMail);
	obj->o_ac = a_class[ArmorType::RingMail] - 1;
	obj->o_flags.set(ISKNOW);
	obj->o_count = 1;
	obj->o_group = 0;
	game().player.armor = obj;
	add_pack(obj, true);
	/*
	 * Give him some food too
	 */
	obj = new_item();
	obj->o_type = ItemKind::Food;
	obj->o_count = 1;
	obj->set_which(Food::Ration);
	obj->o_group = 0;
	add_pack(obj, true);
}

/*
 * Contains definitions and functions for dealing with things like
 * potions and scrolls
 */

constexpr auto rainbow = std::to_array<std::string_view>({
	"amber",
	"aquamarine",
	"black",
	"blue",
	"brown",
	"clear",
	"crimson",
	"cyan",
	"ecru",
	"gold",
	"green",
	"grey",
	"magenta",
	"orange",
	"pink",
	"plaid",
	"purple",
	"red",
	"silver",
	"tan",
	"tangerine",
	"topaz",
	"turquoise",
	"vermilion",
	"violet",
	"white",
	"yellow"
});

constexpr std::size_t NCOLORS = std::size(rainbow);

constexpr std::string_view c_set = "bcdfghjklmnpqrstvwxyz";
constexpr std::string_view v_set = "aeiou";

struct Stone {
	std::string_view st_name;
	int st_value;
};

constexpr auto stones = std::to_array<Stone>({
	{ "agate",		 25},
	{ "alexandrite",	 40},
	{ "amethyst",	 50},
	{ "carnelian",	 40},
	{ "diamond",	300},
	{ "emerald",	300},
	{ "germanium",	225},
	{ "granite",	  5},
	{ "garnet",		 50},
	{ "jade",		150},
	{ "kryptonite",	300},
	{ "lapis lazuli",	 50},
	{ "moonstone",	 50},
	{ "obsidian",	 15},
	{ "onyx",		 60},
	{ "opal",		200},
	{ "pearl",		220},
	{ "peridot",	 63},
	{ "ruby",		350},
	{ "sapphire",	285},
	{ "stibotantalite",	200},
	{ "tiger eye",	 50},
	{ "topaz",		 60},
	{ "turquoise",	 70},
	{ "taaffeite",	300},
	{ "zircon",	 	 80}
});

constexpr std::size_t NSTONES = std::size(stones);

constexpr auto wood = std::to_array<std::string_view>({
	"avocado wood",
	"balsa",
	"bamboo",
	"banyan",
	"birch",
	"cedar",
	"cherry",
	"cinnibar",
	"cypress",
	"dogwood",
	"driftwood",
	"ebony",
	"elm",
	"eucalyptus",
	"fall",
	"hemlock",
	"holly",
	"ironwood",
	"kukui wood",
	"mahogany",
	"manzanita",
	"maple",
	"oaken",
	"persimmon wood",
	"pecan",
	"pine",
	"poplar",
	"redwood",
	"rosewood",
	"spruce",
	"teak",
	"walnut",
	"zebrawood"
});

constexpr std::size_t NWOOD = std::size(wood);

constexpr auto metal = std::to_array<std::string_view>({
	"aluminum",
	"beryllium",
	"bone",
	"brass",
	"bronze",
	"copper",
	"electrum",
	"gold",
	"iron",
	"lead",
	"magnesium",
	"mercury",
	"nickel",
	"pewter",
	"platinum",
	"steel",
	"silver",
	"silicon",
	"tin",
	"titanium",
	"tungsten",
	"zinc"
});

constexpr std::size_t NMETAL = std::size(metal);

/*
 * Make each kind's odds cumulative, the running total that pick_one()
 * compares its roll against
 */
template <typename E>
static void
accumulate_odds(KindTable<E, magic_item> &table)
{
	int odds = 0;

	for (magic_item &mi : table)
		mi.mi_prob = odds += mi.mi_prob;
}

/*
 * init_things
 *	Initialize the probabilities for types of things
 */
void
init_things()
{
	struct magic_item *mp;

	for (mp = &game().items.things[1]; mp <= &game().items.things[NUMTHINGS-1]; mp++)
		mp->mi_prob += (mp-1)->mi_prob;
}

/*
 * init_colors:
 *	Initialize the potion color scheme for this time
 */
void
init_colors()
{
	unsigned int i, j;
	bool used[NCOLORS];
	rogue::Items &items = game().items;

	for (i = 0; i < NCOLORS; i++)
		used[i] = false;
	for (Potion p : kinds<Potion>())
	{
		do
			j = rnd(NCOLORS);
		while (used[j]);
		used[j] = true;
		items.p_colors[p] = rainbow[j];
		items.p_know[p] = false;
		items.p_guess[p].clear();
	}
	accumulate_odds(items.p_magic);
}

/*
 * init_names:
 *	Generate the names of the various scrolls
 */
void
init_names()
{
	rogue::Items &items = game().items;
	 int nsyl;
	 const char *sp;
	 int nwords;

	for (Scroll s : kinds<Scroll>())
	{
	std::string name;
	nwords = rnd(game().options.terse?3:4) + 2;
	while (nwords--)
	{
		nsyl = rnd(2) + 1;
		while (nsyl--)
		{
		sp = getsyl();
		if (name.size() + strlen(sp) > MAXNAME-1)
		{
			nwords = 0;
			break;
		}
		name += sp;
		}
		name += ' ';
	}
	name.pop_back();
	items.s_know[s] = false;
	items.s_guess[s].clear();
	items.s_names[s] = name;
	}
	accumulate_odds(items.s_magic);
}

/*
 * getsyl()
 *   -- generate a random sylable
 */
char*
getsyl()
{
	static char _tsyl[4];

	_tsyl[3] = 0;
	_tsyl[2] = rchr(c_set);
	_tsyl[1] = rchr(v_set);
	_tsyl[0] = rchr(c_set);
	return (_tsyl);
}

/*
 * rchr()
 *    return random character in given string
 */
char
rchr(std::string_view string)
{
	return(string[rnd(string.size())]);
}

/*
 * init_stones:
 *	Initialize the ring stone setting scheme for this time
 */
void
init_stones()
{
	unsigned int i, j;
	bool used[NSTONES];
	rogue::Items &items = game().items;

	for (i = 0; i < NSTONES; i++)
		used[i] = false;
	for (Ring r : kinds<Ring>())
	{
		do
			j = rnd(NSTONES);
		while (used[j]);
		used[j] = true;
		items.r_stones[r] = stones[j].st_name;
		items.r_know[r] = false;
		items.r_guess[r].clear();
		items.r_magic[r].mi_worth += stones[j].st_value;
	}
	accumulate_odds(items.r_magic);
}

/*
 * init_materials:
 *	Initialize the construction materials for wands and staffs
 */
void
init_materials()
{
	unsigned int i, j;
	std::string_view str;
	bool metused[NMETAL], woodused[NWOOD];
	rogue::Items &items = game().items;

	for (i = 0; i < NWOOD; i++)
		woodused[i] = false;
	for (i = 0; i < NMETAL; i++)
		metused[i] = false;
	for (Stick w : kinds<Stick>())
	{
		for (;;)
			if (rnd(2) == 0)
			{
				j = rnd(NMETAL);
				if (!metused[j])
				{
					items.ws_type[w] = "wand";
					str = metal[j];
					metused[j] = true;
					break;
				}
			}
			else
			{
				j = rnd(NWOOD);
				if (!woodused[j])
				{
					items.ws_type[w] = "staff";
					str = wood[j];
					woodused[j] = true;
					break;
				}
			}
		items.ws_made[w] = str;
		items.ws_know[w] = false;
		items.ws_guess[w].clear();
	}
	accumulate_odds(items.ws_magic);
}

/*
 * The experience needed for each level: 10, doubling 18 times, then 0
 * to end the table
 */
const long e_levels[20] = {
	10L, 20L, 40L, 80L, 160L, 320L, 640L, 1280L, 2560L, 5120L, 10240L,
	20480L, 40960L, 81920L, 163840L, 327680L, 655360L, 1310720L, 2621440L, 0L,
};
