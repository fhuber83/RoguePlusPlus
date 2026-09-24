#pragma once

namespace rogue {

class Item;

namespace items {

/*
 * new_thing:
 *	Make a new item, weighted by the per-game odds in game().items and
 *	the food shortage in game().level. Returns null if the item pool is
 *	full (see new_item()).
 */
Item *new_thing();

}  // namespace items
}  // namespace rogue
