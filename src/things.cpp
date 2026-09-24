/*
 * Contains functions for dealing with things like potions, scrolls,
 * and other items.
 *
 * things.c	1.4 (AI Design)	12/14/84
 */

#include "rogue.h"

/*
 * drop:
 *	Put something down
 */
void
drop(void)
{
	byte ch;
	Item *nobj, *op;

	ch = chat(hero.y, hero.x);
	if (ch != FLOOR && ch != PASSAGE)
	{
		msg("there is something there already");
		return;
	}
	if ((op = get_item("drop", ItemFilter::all())) == NULL)
		return;
	if (!can_drop(op))
		return;
	/*
	 * Take it out of the pack
	 */
	if (op->o_count >= 2 && op->o_type != ItemKind::Weapon)
	{
		if ((nobj = new_item()) == NULL)
		{
			msg("%sit appears to be stuck in your pack!",
				noterse("can't drop it, "));
			return;
		}
		op->o_count--;
		bcopy(*nobj,*op);
		nobj->o_count = 1;
		op = nobj;
		if (op->o_group != 0)
			game().player.in_pack++;
	}
	else
		detach(pack, op);
	game().player.in_pack--;
	/*
	 * Link it into the level object list
	 */
	attach(game().level.objects, op);
	chat(hero.y, hero.x) = glyph_of(op->o_type);
	bcopy(op->o_pos,hero);
	if (op->o_type == ItemKind::Amulet)
		game().player.has_amulet = FALSE;
	msg("dropped %s", inv_name(op, TRUE));
}

/*
 * can_drop:
 *	Do special checks for dropping or unweilding|unwearing|unringing
 */
bool
can_drop(Item *op)
{
	rogue::Player &player = game().player;
	if (op == NULL)
		return TRUE;
	if (op != player.armor && op != player.weapon
		&& op != player.rings[LEFT] && op != player.rings[RIGHT])
		return TRUE;
	if (op->o_flags.test(ISCURSED)) {
		msg("you can't.  It appears to be cursed");
		return FALSE;
	}
	if (op == player.weapon)
		player.weapon = NULL;
	else if (op == player.armor) {
		waste_time();
		player.armor = NULL;
	} else {
		int hand;

		if (op != player.rings[hand = LEFT])
			if (op != player.rings[hand = RIGHT]) {
#ifdef DEBUG
				debug("Candrop called with funny thing");
#endif
				return TRUE;
			}
		player.rings[hand] = NULL;
		switch (op->o_which) {
		case R_ADDSTR:
			chg_str(-op->o_ac);
			break;
		case R_SEEINVIS:
			unsee();
			extinguish(unsee);
			break;
		}
	}
	return TRUE;
}

