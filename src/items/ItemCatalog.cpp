#include "items/ItemCatalog.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

#include "core/Config.hpp"
#include "core/KindTable.hpp"
#include "core/Maybe.hpp"
#include "entities/Item.hpp"
#include "game/Game.hpp"
#include "game/Messages.hpp"
#include "game/Pool.hpp"
#include "items/KindInfo.hpp"
#include "items/Kinds.hpp"
#include "items/effects/Wand.hpp"
#include "items/effects/Weapon.hpp"

namespace rogue::items {

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
const KindTable<Scroll, KindInfo> s_magic_base = {
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

const KindTable<Potion, KindInfo> p_magic_base = {
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

const KindTable<Ring, KindInfo> r_magic_base = {
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

const KindTable<Stick, KindInfo> ws_magic_base = {
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
 * A value the game never reads (was ___): the worth of the kinds of item.
 */
constexpr short NA = 1;

/*
 * The odds of each kind of random item. init_things() accumulates them in
 * the game's copy, and the only user is new_thing(). worth is unused (NA).
 */
const std::array<KindInfo, NUMTHINGS> things_base = {{
	{ "",			27, NA },	/* potion */
	{ "",			30, NA },	/* scroll */
	{ "",			17, NA },	/* food */
	{ "",			 8, NA },	/* weapon */
	{ "",			 8, NA },	/* armor */
	{ "",			 5, NA },	/* ring */
	{ "",			 5, NA }	/* stick */
}};

namespace {

/*
 * pick_one:
 *	Pick an item out of a list of possible magic items, by their added-up
 *	odds; the index of the one picked
 */
int
pick_one(std::span<const KindInfo> magic)
{
	int i = rnd(100);

	for (std::size_t n = 0; n < magic.size(); n++)
		if (i < magic[n].prob)
			return static_cast<int>(n);
	if constexpr (rogue::config::debug_checks) {
		debug("bad pick_one: {} from {} items", i, magic.size());
		for (const KindInfo &mi : magic)
			debug("{}: {}%", mi.name, mi.prob);
	}
	return 0;
}

}  // namespace

// Pick a kind of E by the odds in the table
namespace {

template <typename E>
E
pick_one(const KindTable<E, KindInfo> &table)
{
	return static_cast<E>(pick_one(std::span<const KindInfo>(table.data(), table.size())));
}

}  // namespace

/*
 * new_thing:
 *	Return a new thing
 */
Maybe<Item>
new_thing()
{
	rogue::Items &items = game().items;

	Maybe<Item> cur = new_item();
	if (!cur)
		return std::nullopt;
	cur->hit_plus = cur->damage_plus = 0;
	cur->damage = cur->thrown_damage = "0d0";
	cur->ac = 11;
	cur->count = 1;
	cur->group = 0;
	cur->flags.reset();
	cur->enemy = 0;
	/*
	 * Decide what kind of object it will be
	 * If we haven't had food for a while, let it be food.
	 */
	switch (game().level.no_food > 3 ? 2 : pick_one(items.things))
	{
	case 0:
		cur->kind = ItemKind::Potion;
		cur->set_which(pick_one(items.p_magic));
		break;
	case 1:
		cur->kind = ItemKind::Scroll;
		cur->set_which(pick_one(items.s_magic));
		break;
	case 2:
		game().level.no_food = 0;
		cur->kind = ItemKind::Food;
		if (rnd(10) != 0)
			cur->set_which(Food::Ration);
		else
			cur->set_which(Food::Fruit);
		break;
	case 3: {
		cur->kind = ItemKind::Weapon;
		cur->set_which(static_cast<WeaponType>(rnd(kind_count<WeaponType>)));
		items::effects::init_weapon(*cur, cur->which<WeaponType>());
		int k = rnd(100);
		if (k < 10)
		{
			cur->flags.set(ItemFlag::Cursed);
			cur->hit_plus -= rnd(3) + 1;
		}
		else if (k < 15)
			cur->hit_plus += rnd(3) + 1;
		break;
	}
	case 4: {
		std::optional<ArmorType> armor;

		cur->kind = ItemKind::Armor;
		int k = rnd(100);
		for (ArmorType a : kinds<ArmorType>())
			if (k < a_chances[a]) {
				armor = a;
				break;
			}
		if (!armor)
		{
			if constexpr (rogue::config::debug_checks)
				debug("Picked a bad armor {}", k);
			armor = ArmorType::Leather;
		}
		cur->set_which(*armor);
		cur->ac = a_class[*armor];
		if ((k = rnd(100)) < 20)
		{
			cur->flags.set(ItemFlag::Cursed);
			cur->ac += rnd(3) + 1;
		}
		else if (k < 28)
			cur->ac -= rnd(3) + 1;
		break;
	}
	case 5:
		cur->kind = ItemKind::Ring;
		cur->set_which(pick_one(items.r_magic));
		switch (cur->which<Ring>())
		{
		case Ring::AddStrength:
		case Ring::Protection:
		case Ring::Dexterity:
		case Ring::IncreaseDamage:
			if ((cur->ac = rnd(3)) == 0)
			{
				cur->ac = -1;
				cur->flags.set(ItemFlag::Cursed);
			}
			break;
		case Ring::AggravateMonster:
		case Ring::Teleportation:
			cur->flags.set(ItemFlag::Cursed);
			break;
		default:
			break;
		}
		break;
	case 6:
		cur->kind = ItemKind::Stick;
		cur->set_which(pick_one(items.ws_magic));
		items::effects::fix_stick(*cur);
		break;
	default:
		if constexpr (rogue::config::debug_checks) {
			debug("Picked a bad kind of object");
			wait_for(' ');
		}
		break;
	}
	return cur;
}

}  // namespace rogue::items
