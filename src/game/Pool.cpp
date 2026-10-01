/*
 * Taking creatures and items from the pool and giving them back. (The
 * lists themselves are rogue::List, entities/List.hpp.)
 *
 * list.c	1.4 (A.I. Design) 12/5/85
 */

#include "rogue.h"

namespace rogue {

namespace {

/*
 * talloc: take the first free slot of a pool. The two pools share one count,
 * like the single pool of the original (see rogue::Pool).
 */
template <class T>
Maybe<T>
talloc(Slots<T, MAXITEMS> &slots)
{
	Pool &pool = game().pool;
	T *thing;

	if (pool.total >= MAXITEMS || (thing = slots.take()) == nullptr)
		return std::nullopt;
	++pool.total;
	return *thing;
}

/*
 * discard: give a slot back to its pool, which destroys what it held
 */
template <class T>
bool
discard_from(T *item, Slots<T, MAXITEMS> &slots)
{
	if (!slots.release(item))
		return false;
	--game().pool.total;
	return true;
}

}  // namespace

/*
 * new_item
 *	Get a new item from the pool
 */
Maybe<Item>
new_item()
{
	return talloc(game().pool.items);
}

// new_item() for monsters
Maybe<Creature>
new_creature()
{
	return talloc(game().pool.creatures);
}

/*
 * discard:
 *	Free up an item
 */
bool
discard(Item &item)
{
	/*
	 * get_item() compares the item it gave last with the one at that pack
	 * letter. The original kept pointing at the freed slot, which matched a
	 * new item that reused the slot; a freed item's address can be reused
	 * too, so forget it.
	 */
	if (refers_to(game().pool.item(game().turn.last_item), item))
		game().turn.last_item = std::nullopt;
	/*
	 * A monster after this item goes for the hero instead. add_pack() does
	 * this when the rogue picks the item up, but not when it merges into a
	 * pack item and is discarded: the original then chased the freed slot's
	 * old position until the slot was reused.
	 */
	if (std::optional<ItemId> id = game().pool.id_of(item))
		for (Creature &mp : game().level.monsters)
			if (mp.t_dest == Destination(*id))
				mp.t_dest = Hero{};
	return discard_from(&item, game().pool.items);
}

bool
discard(Creature &item)
{
	return discard_from(&item, game().pool.creatures);
}

}  // namespace rogue
