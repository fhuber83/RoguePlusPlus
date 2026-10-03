#pragma once

/*
 * What items there are: the names and odds of the kinds of item, the
 * weapon and armor tables, and making a new random item.
 */

#include <array>
#include <string_view>

#include "core/KindTable.hpp"
#include "core/Maybe.hpp"
#include "items/KindInfo.hpp"
#include "items/Kinds.hpp"

namespace rogue {

struct Item;

namespace items {

/*
 * The name, odds and worth of each kind. Each game works on a copy in
 * game().items, since init_*() accumulate the odds and add the stone value
 * to the worth of rings.
 */
extern const KindTable<Scroll, KindInfo> s_magic_base;
extern const KindTable<Potion, KindInfo> p_magic_base;
extern const KindTable<Ring, KindInfo> r_magic_base;
extern const KindTable<Stick, KindInfo> ws_magic_base;
// The odds of each type of item, in the order new_thing() picks from
extern const std::array<KindInfo, NUMTHINGS> things_base;

// Weapon names, and the name of the WeaponType::Flame that fire_bolt() throws
extern KindTable<WeaponType, std::string_view, kind_count<WeaponType> + 1> w_names;
extern const KindTable<ArmorType, std::string_view> a_names;
// The chance of each armor type, cumulative out of 100
extern const KindTable<ArmorType, int> a_chances;
// The armor class of each armor type, unenchanted
extern const KindTable<ArmorType, int> a_class;

/*
 * new_thing:
 *	Make a new item, weighted by the per-game odds in game().items and
 *	the food shortage in game().level. Returns null if the item pool is
 *	full (see new_item()).
 */
Maybe<Item> new_thing();

}  // namespace items
}  // namespace rogue
