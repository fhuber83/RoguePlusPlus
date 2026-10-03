/*
 * What an item is: the questions asked of one.
 *
 * is_magic() comes from fight.c.
 */

#include "entities/Item.hpp"

#include "items/ItemCatalog.hpp"
#include "items/Kinds.hpp"

namespace rogue {

bool
Item::is_magic() const
{
	switch (kind)
	{
	case ItemKind::Armor:
		return ac != items::a_class[which<ArmorType>()];
	case ItemKind::Weapon:
		return hit_plus != 0 || damage_plus != 0;
	case ItemKind::Potion:
	case ItemKind::Scroll:
	case ItemKind::Stick:
	case ItemKind::Ring:
	case ItemKind::Amulet:
		return true;
	default:	// the other kinds of item: nothing
		break;
	}
	return false;
}

}  // namespace rogue
