#include "rogue.h"

namespace rogue::items {

/*
 * pick_one:
 *	Pick an item out of a list of nitems possible magic items
 */
static
int  // actually an offset, the element index in the array
pick_one(struct magic_item *magic, int nitems)
{
	struct magic_item *end;
	int i;
	struct magic_item *start;

	start = magic;
	for (end = &magic[nitems], i = rnd(100); magic < end; magic++)
		if (i < magic->mi_prob)
			break;
	if (magic == end)
	{
#ifdef DEBUG
		if (wizard)
		{
			msg("bad pick_one: {} from {} items", i, nitems);
			for (magic = start; magic < end; magic++)
				msg("{}: {}%", magic->mi_name, magic->mi_prob);
		}
#endif
		magic = start;
	}
	return magic - start;
}

/*
 * new_thing:
 *	Return a new thing
 */
Item *
new_thing()
{
	Item *cur;
	int j, k;
	rogue::Items &items = game().items;

	if ((cur = new_item()) == NULL)
		return NULL;
	cur->o_hplus = cur->o_dplus = 0;
	cur->o_damage = cur->o_hurldmg = "0d0";
	cur->o_ac = 11;
	cur->o_count = 1;
	cur->o_group = 0;
	cur->o_flags.reset();
	cur->o_enemy = 0;
	/*
	 * Decide what kind of object it will be
	 * If we haven't had food for a while, let it be food.
	 */
	switch (game().level.no_food > 3 ? 2 : pick_one(items.things, NUMTHINGS))
	{
	case 0:
		cur->o_type = ItemKind::Potion;
		cur->o_which = pick_one(items.p_magic, MAXPOTIONS);
		break;
	case 1:
		cur->o_type = ItemKind::Scroll;
		cur->o_which = pick_one(items.s_magic, MAXSCROLLS);
		break;
	case 2:
		game().level.no_food = 0;
		cur->o_type = ItemKind::Food;
		if (rnd(10) != 0)
			cur->o_which = 0;
		else
			cur->o_which = 1;
		break;
	case 3:
		cur->o_type = ItemKind::Weapon;
		cur->o_which = rnd(MAXWEAPONS);
		init_weapon(cur, cur->o_which);
		if ((k = rnd(100)) < 10)
		{
			cur->o_flags.set(ISCURSED);
			cur->o_hplus -= rnd(3) + 1;
		}
		else if (k < 15)
			cur->o_hplus += rnd(3) + 1;
		break;
	case 4:
		cur->o_type = ItemKind::Armor;
		for (j = 0, k = rnd(100); j < MAXARMORS; j++)
			if (k < a_chances[j])
				break;
#ifdef DEBUG
		if (j == MAXARMORS)
		{
		debug("Picked a bad armor {}", k);
		j = 0;
		}
#endif
		cur->o_which = j;
		cur->o_ac = a_class[j];
		if ((k = rnd(100)) < 20)
		{
			cur->o_flags.set(ISCURSED);
			cur->o_ac += rnd(3) + 1;
		}
		else if (k < 28)
			cur->o_ac -= rnd(3) + 1;
		break;
	case 5:
		cur->o_type = ItemKind::Ring;
		cur->o_which = pick_one(items.r_magic, MAXRINGS);
		switch (cur->o_which)
		{
		case R_ADDSTR:
		case R_PROTECT:
		case R_ADDHIT:
		case R_ADDDAM:
			if ((cur->o_ac = rnd(3)) == 0)
			{
				cur->o_ac = -1;
				cur->o_flags.set(ISCURSED);
			}
			break;
		case R_AGGR:
		case R_TELEPORT:
			cur->o_flags.set(ISCURSED);
			break;
		}
		break;
	case 6:
		cur->o_type = ItemKind::Stick;
		cur->o_which = pick_one(items.ws_magic, MAXSTICKS);
		fix_stick(cur);
		break;
#ifdef DEBUG
	default:
		debug("Picked a bad kind of object");
		wait_for(' ');
		break;
#endif
	}
	return cur;
}

}  // namespace rogue::items
