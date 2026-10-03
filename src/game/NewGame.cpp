/*
 * Setting up a new game: the rogue's first pack, and the odds and looks of
 * the kinds of item.
 *
 * init.c	1.4 (A.I. Design) 12/14/84
 * setup() and credits() come from mach_dep.c (1.4 (A.I. Design) 12/1/84).
 */

#include "game/NewGame.hpp"

#include <array>
#include <cstddef>
#include <iterator>
#include <span>
#include <string>
#include <string_view>

#include "core/KindTable.hpp"
#include "core/Maybe.hpp"
#include "entities/Item.hpp"
#include "game/Game.hpp"
#include "game/Pool.hpp"
#include "items/Inventory.hpp"
#include "items/ItemCatalog.hpp"
#include "items/KindInfo.hpp"
#include "items/Kinds.hpp"
#include "items/effects/Weapon.hpp"
#include "rules/Durations.hpp"
#include "ui/Display.hpp"
#include "ui/Input.hpp"

namespace rogue {

namespace {

/*
 * What the kinds of potion, ring and stick can look like, and the letters
 * of scroll titles; each game picks its own (see Items)
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
void
accumulate_odds(KindTable<E, items::KindInfo> &table)
{
	int odds = 0;

	for (items::KindInfo &mi : table)
		mi.mi_prob = odds += mi.mi_prob;
}

}  // namespace

/*
 * setup:
 *	Get starting setup for all games
 */
void
setup()
{
	game().options.terse = false;
	game().options.expert = game().options.terse;
}

/*
 * credits:
 *	Show the title screen and ask for the rogue's name
 */
void
credits()
{
	ui::display().draw_title();
	if (auto name = ui::input().read_line(Options::name_length); name && !name->empty())
		game().options.name = *name;
	ui::display().end_title();
}

/*
 * init_player:
 *	Roll up the rogue
 */
void
init_player()
{
	game().player.body.t_stats = game().player.max_stats;
	game().player.food_left = rules::hunger_time();
	/*
	 * initialize things
	 */
	game().pool = rogue::Pool();
	/*
	 * Give the rogue his weaponry.  First a mace.
	 */
	Maybe<Item> obj = new_item();
	obj->o_type = ItemKind::Weapon;
	obj->set_which(WeaponType::Mace);
	items::effects::init_weapon(*obj, WeaponType::Mace);
	obj->o_hplus = 1;
	obj->o_dplus = 1;
	obj->o_flags.set(ItemFlag::Known);
	obj->o_count = 1;
	obj->o_group = 0;
	items::add_pack(*obj, true);
	game().player.weapon = game().pool.id_of(obj);
	/*
	 * Now a +1 bow
	 */
	obj = new_item();
	obj->o_type = ItemKind::Weapon;
	obj->set_which(WeaponType::ShortBow);
	items::effects::init_weapon(*obj, WeaponType::ShortBow);
	obj->o_hplus = 1;
	obj->o_dplus = 0;
	obj->o_count = 1;
	obj->o_group = 0;
	obj->o_flags.set(ItemFlag::Known);
	items::add_pack(*obj, true);
	/*
	 * Now some arrows
	 */
	obj = new_item();
	obj->o_type = ItemKind::Weapon;
	obj->set_which(WeaponType::Arrow);
	items::effects::init_weapon(*obj, WeaponType::Arrow);
	obj->o_count = rnd(15) + 25;
	obj->o_hplus = obj->o_dplus = 0;
	obj->o_flags.set(ItemFlag::Known);
	items::add_pack(*obj, true);
	/*
	 * And his suit of armor
	 */
	obj = new_item();
	obj->o_type = ItemKind::Armor;
	obj->set_which(ArmorType::RingMail);
	obj->o_ac = items::a_class[ArmorType::RingMail] - 1;
	obj->o_flags.set(ItemFlag::Known);
	obj->o_count = 1;
	obj->o_group = 0;
	game().player.armor = game().pool.id_of(obj);
	items::add_pack(*obj, true);
	/*
	 * Give him some food too
	 */
	obj = new_item();
	obj->o_type = ItemKind::Food;
	obj->o_count = 1;
	obj->set_which(Food::Ration);
	obj->o_group = 0;
	items::add_pack(*obj, true);
}

/*
 * init_things
 *	Initialize the probabilities for types of things
 */
void
init_things()
{
	std::span<items::KindInfo> things = game().items.things;

	for (std::size_t i = 1; i < things.size(); i++)
		things[i].mi_prob += things[i-1].mi_prob;
}

/*
 * init_colors:
 *	Initialize the potion color scheme for this time
 */
void
init_colors()
{
	std::array<bool, NCOLORS> used{};
	rogue::Items &items = game().items;

	for (Potion p : kinds<Potion>())
	{
		unsigned int j;
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

	for (Scroll s : kinds<Scroll>())
	{
		std::string name;
		int nwords = rnd(game().options.terse?3:4) + 2;
		while (nwords--)
		{
			int nsyl = rnd(2) + 1;
			while (nsyl--)
			{
				std::string sp = getsyl();
				if (name.size() + sp.size() > MAXNAME-1)
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
std::string
getsyl()
{
	// Drawn last letter first, as the original filled its buffer
	char last = rchr(c_set);
	char vowel = rchr(v_set);
	char first = rchr(c_set);
	return {first, vowel, last};
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
	std::array<bool, NSTONES> used{};
	rogue::Items &items = game().items;

	for (Ring r : kinds<Ring>())
	{
		unsigned int j;
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
	std::array<bool, NMETAL> metused{};
	std::array<bool, NWOOD> woodused{};
	rogue::Items &items = game().items;

	for (Stick w : kinds<Stick>())
	{
		std::string_view str;
		for (;;)
			if (rnd(2) == 0)
			{
				unsigned int j = rnd(NMETAL);
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
				unsigned int j = rnd(NWOOD);
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

}  // namespace rogue
