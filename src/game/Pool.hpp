#pragma once

/*
 * Taking creatures and items from the pool (game().pool) and giving them
 * back.
 */

#include "core/Maybe.hpp"
#include "entities/List.hpp"

namespace rogue {

struct Item;
struct Creature;

/*
 * new_item, new_creature:
 *	Take a new thing from the pool's first free slot, or nothing if the
 *	pool already holds MAXITEMS things of both kinds.
 */
Maybe<Item> new_item();
Maybe<Creature> new_creature();

/*
 * discard:
 *	Give a thing back to the pool, which destroys it, so it must not be
 *	read afterwards; take it out of its list first. Forgets the item as
 *	the last one picked, and sends a monster after it to the hero instead.
 *	False if the thing is not in the pool.
 */
bool discard(Item &item);
bool discard(Creature &item);

/*
 * list_free:
 *	Empty a list of creatures or items and give them back to the pool.
 */
template <class T>
void
list_free(List<T> &list)
{
	while (Maybe<T> item = list.first()) {
		list.remove(*item);
		discard(*item);
	}
}

}  // namespace rogue
