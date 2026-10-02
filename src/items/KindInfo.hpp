#pragma once

#include <string_view>

namespace rogue::items {

/*
 * A kind of potion, scroll, ring or stick, or a type of item: what it is
 * called, its odds of turning up and what it is worth (was struct
 * magic_item). The fixed tables (items/ItemCatalog.hpp) hold each kind's own
 * odds; init_*() make the game's copies cumulative.
 */
struct KindInfo {
	std::string_view mi_name;
	int mi_prob;
	short mi_worth;
};

}  // namespace rogue::items
