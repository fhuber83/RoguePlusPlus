#pragma once

/*
 * Naming and display of items: the inventory string for one item, and the
 * "discovered" screen listing what the player has identified so far.
 *
 * Included by rogue.h after entities/Item.hpp (Item).
 */

namespace rogue {

class Item;

namespace items {

/*
 * inv_name:
 *	Return the name of something as it would appear in an inventory.
 */
std::string inv_name(const Item *obj, bool drop);

/*
 * discovered:
 *	List what the player has discovered in this game, by kind.
 */
void discovered();

/*
 * add_line/end_line:
 *	Build a paged list of lines (inventory, discoveries) through the
 *	display, one call per line. end_line closes the page.
 */
unsigned char add_line(std::string_view use, std::optional<std::string_view> line);
unsigned char end_line(std::string_view use);

}  // namespace items
}  // namespace rogue
