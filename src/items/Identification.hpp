#pragma once

/*
 * Naming and display of items: the inventory string for one item, and the
 * "discovered" screen listing what the player has identified so far.
 */

#include <optional>
#include <string>
#include <string_view>

namespace rogue {

struct Item;

namespace items {

/*
 * inv_name:
 *	Return the name of something as it would appear in an inventory.
 */
std::string inv_name(const Item &obj, bool drop);

/*
 * discovered:
 *	List what the player has discovered in this game, by kind.
 */
void discovered();

/*
 * Pager:
 *	A paged list of lines (inventory, discoveries) through the display,
 *	one add_line() per line; end_line() waits for a key and closes the
 *	page. Both return the key that ended a page (' ' if none did yet).
 */
class Pager {
public:
	unsigned char add_line(std::string_view use, std::optional<std::string_view> line);
	unsigned char end_line(std::string_view use);

private:
	int line_cnt = 0;	/* the next line of the page */
};

/*
 * call_it:
 *	After using an item: forget the guess once the kind is known, or ask
 *	what to call a kind that is neither known nor guessed.
 */
void call_it(bool know, std::string &guess);

/*
 * call:
 *	The call command: name a kind of potion, scroll, ring or wand that is
 *	not identified yet.
 */
void call();

/*
 * whatis:
 *	Identify a kind of item the rogue picks from his pack (the scroll of
 *	identify); he must pick one.
 */
void whatis();

}  // namespace items
}  // namespace rogue
