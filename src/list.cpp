/*
 * Functions for dealing with linked lists of goodies
 * Functions with names starting with an "_" have compainion #defines
 * in rogue.h which take the address of the first argument and pass it on.
 *
 * list.c	1.4 (A.I. Design) 12/5/85
 */

#include "rogue.h"

/*
 * The lists themselves are rogue::List (entities/List.hpp) now. What is left
 * here takes things from the pool and gives them back.
 */

/*
 * talloc: take the first free slot of a pool. The two pools share one count,
 * like the single pool of the original (see rogue::Pool).
 */
template <class T>
static T *
talloc(rogue::Slots<T, MAXITEMS> &slots)
{
	rogue::Pool &pool = game().pool;
	T *thing;

	if (pool.total >= MAXITEMS || (thing = slots.take()) == NULL)
		return NULL;
	++pool.total;
	return thing;
}

/*
 * new_item
 *	Get a new item from the pool
 */
Item *
new_item()
{
	return talloc(game().pool.items);
}

// new_item() for monsters
Creature *
new_creature()
{
	return talloc(game().pool.creatures);
}

/*
 * discard: give a slot back to its pool, which destroys what it held
 */
template <class T>
static int
discard_from(T *item, rogue::Slots<T, MAXITEMS> &slots)
{
	if (!slots.release(item))
		return 0;
	--game().pool.total;
	return 1;
}

/*
 * discard:
 *	Free up an item
 */
int
discard(Item *item)
{
	/*
	 * get_item() compares the item it gave last with the one at that pack
	 * letter. The original kept pointing at the freed slot, which matched a
	 * new item that reused the slot; a freed item's address can be reused
	 * too, so forget it.
	 */
	if (game().turn.last_item == item)
		game().turn.last_item = NULL;
	/*
	 * A monster after this item goes for the hero instead. add_pack() does
	 * this when the rogue picks the item up, but not when it merges into a
	 * pack item and is discarded: the original then chased the freed slot's
	 * old position until the slot was reused.
	 */
	for (Creature *mp : game().level.monsters)
		if (mp->t_dest == &item->o_pos)
			mp->t_dest = &hero;
	return discard_from(item, game().pool.items);
}

int
discard(Creature *item)
{
	return discard_from(item, game().pool.creatures);
}
