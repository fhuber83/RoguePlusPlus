#include "rogue.h"

namespace rogue::items {

/*
 * pick_one:
 *	Pick an item out of a list of possible magic items, by their added-up
 *	odds; the index of the one picked
 */
static int
pick_one(std::span<const magic_item> magic)
{
	int i = rnd(100);

	for (std::size_t n = 0; n < magic.size(); n++)
		if (i < magic[n].mi_prob)
			return static_cast<int>(n);
	if constexpr (rogue::config::debug_checks) {
		debug("bad pick_one: {} from {} items", i, magic.size());
		for (const magic_item &mi : magic)
			debug("{}: {}%", mi.mi_name, mi.mi_prob);
	}
	return 0;
}

// Pick a kind of E by the odds in the table
template <typename E>
static E
pick_one(const KindTable<E, magic_item> &table)
{
	return static_cast<E>(pick_one(std::span<const magic_item>(table.data(), table.size())));
}

/*
 * new_thing:
 *	Return a new thing
 */
Item *
new_thing()
{
	Item *cur;
	int k;
	rogue::Items &items = game().items;

	if ((cur = new_item()) == nullptr)
		return nullptr;
	cur->o_hplus = cur->o_dplus = 0;
	cur->o_damage = cur->o_hurldmg = "0d0";
	cur->o_ac = 11;
	cur->o_count = 1;
	cur->o_group = 0;
	cur->o_flags.reset();
	cur->o_enemy = 0;
	/*
	 * Decide what kind of object it will be
	 * If we haven't had food for a while, let it be food.
	 */
	switch (game().level.no_food > 3 ? 2 : pick_one(items.things))
	{
	case 0:
		cur->o_type = ItemKind::Potion;
		cur->set_which(pick_one(items.p_magic));
		break;
	case 1:
		cur->o_type = ItemKind::Scroll;
		cur->set_which(pick_one(items.s_magic));
		break;
	case 2:
		game().level.no_food = 0;
		cur->o_type = ItemKind::Food;
		if (rnd(10) != 0)
			cur->set_which(Food::Ration);
		else
			cur->set_which(Food::Fruit);
		break;
	case 3:
		cur->o_type = ItemKind::Weapon;
		cur->set_which(static_cast<WeaponType>(rnd(kind_count<WeaponType>)));
		init_weapon(*cur, cur->which<WeaponType>());
		if ((k = rnd(100)) < 10)
		{
			cur->o_flags.set(ISCURSED);
			cur->o_hplus -= rnd(3) + 1;
		}
		else if (k < 15)
			cur->o_hplus += rnd(3) + 1;
		break;
	case 4: {
		std::optional<ArmorType> armor;

		cur->o_type = ItemKind::Armor;
		k = rnd(100);
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
		cur->o_ac = a_class[*armor];
		if ((k = rnd(100)) < 20)
		{
			cur->o_flags.set(ISCURSED);
			cur->o_ac += rnd(3) + 1;
		}
		else if (k < 28)
			cur->o_ac -= rnd(3) + 1;
		break;
	}
	case 5:
		cur->o_type = ItemKind::Ring;
		cur->set_which(pick_one(items.r_magic));
		switch (cur->which<Ring>())
		{
		case Ring::AddStrength:
		case Ring::Protection:
		case Ring::Dexterity:
		case Ring::IncreaseDamage:
			if ((cur->o_ac = rnd(3)) == 0)
			{
				cur->o_ac = -1;
				cur->o_flags.set(ISCURSED);
			}
			break;
		case Ring::AggravateMonster:
		case Ring::Teleportation:
			cur->o_flags.set(ISCURSED);
			break;
		default:
			break;
		}
		break;
	case 6:
		cur->o_type = ItemKind::Stick;
		cur->set_which(pick_one(items.ws_magic));
		fix_stick(*cur);
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
