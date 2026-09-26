#pragma once

/*
 * The pack: picking things up, dropping them, listing them and picking one
 * out for a command.
 *
 * Included by rogue.h after entities/Item.hpp (Item, ItemFilter) and
 * entities/List.hpp (List).
 */

namespace rogue {

class Item;
class ItemFilter;
template <typename T> class List;

namespace items {

/*
 * add_pack:
 *	Pick up an object and add it to the pack. If obj is non-null, use it
 *	instead of getting one off the floor.
 */
void add_pack(Item *obj, bool silent);

/*
 * pick_up:
 *	Add something to the character's pack, or gold to the purse.
 */
void pick_up(unsigned char ch);

/*
 * get_item:
 *	Pick something out of a pack for a purpose (prompts the player).
 */
Item *get_item(const char *purpose, ItemFilter type);

/*
 * inventory:
 *	List what is in a pack (or another item list) through a page.
 */
unsigned char inventory(const List<Item> &list, ItemFilter type, const char *lstr);

/*
 * pack_char:
 *	Return which character would address a pack object.
 */
unsigned char pack_char(Item *obj);

/*
 * money:
 *	Add or subtract gold from the purse.
 */
void money(int value);

/*
 * drop:
 *	Put something down.
 */
void drop();

/*
 * can_drop:
 *	Do special checks for dropping or unwielding/unwearing/unringing.
 */
bool can_drop(Item *op);

}  // namespace items
}  // namespace rogue
