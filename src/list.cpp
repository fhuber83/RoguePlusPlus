/*
 * Functions for dealing with linked lists of goodies
 * Functions with names starting with an "_" have compainion #defines
 * in rogue.h which take the address of the first argument and pass it on.
 *
 * list.c	1.4 (A.I. Design) 12/5/85
 */

#include "rogue.h"

/*@
 * The list functions are templates in rogue.h now, for both Creature and
 * Item lists.
 */

/*@
 * talloc: take a free slot of a pool. The two pools share one count, like
 * the single pool of the original (see rogue::Pool).
 */
template <class T>
static T *
talloc(T *slots, bool *used)
{
	int i;
	rogue::Pool &pool = game().pool;

	if (pool.total >= MAXITEMS)
		return NULL;
	for (i=0;i<MAXITEMS;i++)
	{
		if (!used[i])
		{
			++pool.total;
			used[i] = true;
			slots[i] = T{};
			return &slots[i];
		}
	}
	return NULL;
}

/*
 * new_item
 *	Get a new item with a specified size
 *	@ items and creatures come from separate pools now
 */
Item *
new_item()
{
	return talloc(game().pool.items, game().pool.item_used);
}

//@ new_item() for monsters
Creature *
new_creature()
{
	return talloc(game().pool.creatures, game().pool.creature_used);
}

/*@
 * discard: give a slot back to its pool
 */
template <class T>
static int
discard_from(T *item, T *slots, bool *used)
{
	int i;

	for (i=0;i<MAXITEMS;i++)
	{
		if (item == &slots[i])
		{
			--game().pool.total;
			used[i] = false;
			return 1;
		}
	}
	return 0;
}

/*
 * discard:
 *	Free up an item
 */
int
discard(Item *item)
{
	return discard_from(item, game().pool.items, game().pool.item_used);
}

int
discard(Creature *item)
{
	return discard_from(item, game().pool.creatures, game().pool.creature_used);
}
