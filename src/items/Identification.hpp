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
char *inv_name(Item *obj, bool drop);

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
unsigned char add_line(const char *use, const char *fmt, const char *arg);
unsigned char end_line(const char *use);

}  // namespace items
}  // namespace rogue
