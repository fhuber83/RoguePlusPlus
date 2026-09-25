#include "rogue.h"

namespace rogue::items {

static
Item *
pack_obj(unsigned char ch, unsigned char *chp)
{
	Item *obj;
	unsigned char och;

	for (obj = pack.first(), och = 'a'; obj != NULL; obj = pack.after(obj), och++)
		if (ch == och)
			return obj;
	*chp = och;
	return NULL;
}

/*
 * add_pack:
 *	Pick up an object and add it to the pack.  If the argument is
 *	non-null use it as the linked_list pointer instead of gettting
 *	it off the ground.
 */
void
add_pack(Item *obj, bool silent)
{
	Item *op, *lp = NULL;
	Creature *mp;
	bool exact, from_floor;
	unsigned char floor;

	if (obj == NULL)
	{
		from_floor = TRUE;
		if ((obj = find_obj(hero.y, hero.x)) == NULL)
			return;
	}
	else
		from_floor = FALSE;
	/*
	 * Link it into the pack.  Search the pack for a object of similar type
	 * if there isn't one, stuff it at the beginning, if there is, look for one
	 * that is exactly the same and just increment the count if there is.
	 * Food is always put at the beginning for ease of access, but it
	 * is not ordered so that you can't tell good food from bad.  First check
	 * to see if there is something in the same group and if there is then
	 * increment the count.
	 */

	/*@
	 *  bug in original Rogue: it didn't check proom != NULL, as is the case
	 *  when add_pack() is called from init_player(), which happens before
	 *  any room even exist. proom is set in enter_room(), which is first
	 *  called in new_level()
	 */
	floor = (proom != NULL && proom->r_flags.test(RoomFlag::Gone)) ? PASSAGE : FLOOR;
	if (obj->o_group)
	{
		for (op = pack.first(); op != NULL; op = pack.after(op))
		{
			if (op->o_group == obj->o_group)
			{
			/*
			 * Put it in the pack and notify the user
			 */
				op->o_count += obj->o_count;
				if (from_floor)
				{
					detach(game().level.objects, obj);
					display().draw_tile(hero, floor);
					chat(hero.y, hero.x) = floor;
				}
				discard(obj);
				obj = op;
				goto picked_up;
			}
		}
	}
	/*
	 * Check if there is room
	 */
	if (game().player.in_pack >= MAXPACK-1)
	{
		msg("you can't carry anything else");
		return;
	}
	/*
	 * Check for and deal with scare monster scrolls
	 */
	if (obj->o_type == ItemKind::Scroll && obj->o_which == S_SCARE)
	{
		if (obj->o_flags.test(rogue::ItemFlag::Found))
		{
			detach(game().level.objects, obj);
			display().draw_tile(hero, floor);
			chat(hero.y, hero.x) = floor;
			msg("the scroll turns to dust%s.", noterse(" as you pick it up"));
			return;
		}
		else
			obj->o_flags.set(rogue::ItemFlag::Found);
	}

	game().player.in_pack++;
	if (from_floor)
	{
		detach(game().level.objects, obj);
		display().draw_tile(hero, floor);
		chat(hero.y, hero.x) = floor;
	}
	/*
	 * Search for an object of the same type
	 */
	exact = FALSE;
	for (op = pack.first(); op != NULL; op = pack.after(op))
		if (obj->o_type == op->o_type)
			break;
	if (op == NULL)
	{
		/*
		 * Put it at the end of the pack since it is a new type
		 */
		for (op = pack.first(); op != NULL; op = pack.after(op))
		{
			if (op->o_type != ItemKind::Food)
				break;
			lp = op;
		}
	}
	else
	{
		/*
		 * Search for an object which is exactly the same
		 */
		while (op->o_type == obj->o_type)
		{
			if (op->o_which == obj->o_which)
			{
				exact = TRUE;
				break;
			}
			lp = op;
			if ((op = pack.after(op)) == NULL)
				break;
		}
	}
	if (op == NULL)
	{
		/*
		 * Didn't find an exact match, just stick it here
		 */
		pack.insert_after(lp, obj);	//@ lp is NULL only when the pack is empty
	}
	else
	{
		/*
		 * If we found an exact match.  If it is a potion, food, or a
		 * scroll, increase the count, otherwise put it with its clones.
		 */
		if (exact && ISMULT(obj->o_type))
		{
			op->o_count++;
			discard(obj);
			obj = op;
			goto picked_up;
		}
		pack.insert_before(op, obj);
	}
picked_up:
	/*
	 * If this was the object of something's desire, that monster will
	 * get mad and run at the hero
	 */
	for (mp = game().level.monsters.first(); mp != NULL; mp = game().level.monsters.after(mp))
	{
		/*
		 *  compiler bug: jll : 2-7-83
		 *		It is stupid because it thinks the obj... is not an lvalue
		 *		this may be true since there is no structure assignments,
		 *		but still it should let you have the address??!!
		 *
		if (&obj->_o._o_pos == mp->t_dest)
		 *
		 *  the following should do the same
		 */
		/*@
		 * Another bug in Rogue: missed NULL check for t_dest. Monsters could
		 * be not chasing (sleeping, another room, Ice Monster, etc), so a
		 * destination could possibly have never been assigned.
		 */
		if (mp->t_dest != NULL &&
		   (mp->t_dest->x == obj->o_pos.x) && (mp->t_dest->y == obj->o_pos.y))
			mp->t_dest = &hero;
	}

	if (obj->o_type == ItemKind::Amulet)
	{
		game().player.has_amulet = TRUE;
		game().player.saw_amulet = TRUE;
	}
	/*
	 * Notify the user
	 */
	if (!silent)
		msg("%s%s (%c)",noterse("you now have "),
			inv_name(obj, TRUE).c_str(), pack_char(obj));
}

/*
 * inventory:
 *	List what is in the pack
 */
unsigned char
inventory(const List<Item> &list, ItemFilter type, const char *lstr)
{
	unsigned char ch;
	Item *obj;
	int n_objs;

	n_objs = 0;
	for (ch = 'a', obj = list.first(); obj != NULL; ch++, obj = list.after(obj))
	{
		/*
		 * Don't print this one if:
		 *	the type doesn't match the type we were passed AND
		 *	it isn't a callable type AND
		 *	it isn't a zappable weapon
		 */
		if (!type.is_all() && !type.is(obj->o_type) && !(type.is_callable() &&
		  (obj->o_type == ItemKind::Scroll || obj->o_type == ItemKind::Potion ||
		  obj->o_type == ItemKind::Ring || obj->o_type == ItemKind::Stick)) &&
		  !(type.is(ItemKind::Weapon) && obj->o_type == ItemKind::Potion) &&
		  !(type.is(ItemKind::Stick) && obj->o_enemy && obj->o_charges))
			continue;
		n_objs++;
		add_line(lstr, std::format("{}) {}", static_cast<char>(ch), inv_name(obj, FALSE)).c_str());
	}
	if (n_objs == 0)
	{
		msg(type.is_all() ? "you are empty handed" :
					"you don't have anything appropriate");
		return 0;
	}
	return(end_line(lstr));
}

/*
 * pick_up:
 *	Add something to characters pack.
 */
void
pick_up(unsigned char ch)
{
	Item *obj;

	switch (ch)
	{
	case GOLD:
	{
		Creature *mp;

		if ((obj = find_obj(hero.y, hero.x)) == NULL)
		return;
		money(obj->o_goldval);
		/*@
		 * find_dest() can point a monster's t_dest straight at this gold's
		 * o_pos. Redirect it to the hero before the gold's pool slot is
		 * discarded, same as add_pack()'s "picked_up" redirect for other
		 * floor items, so nothing is left pointing at a freed Item.
		 */
		for (mp = game().level.monsters.first(); mp != NULL; mp = game().level.monsters.after(mp))
			if (mp->t_dest != NULL &&
			   (mp->t_dest->x == obj->o_pos.x) && (mp->t_dest->y == obj->o_pos.y))
				mp->t_dest = &hero;
		detach(game().level.objects, obj);
		discard(obj);
		proom->r_goldval = 0;
		break;
	}
	default:
	case ARMOR:
	case POTION:
	case FOOD:
	case WEAPON:
	case SCROLL:
	case AMULET:
	case RING:
	case STICK:
		add_pack(NULL, FALSE);
		break;
	}
}

/*
 * get_item:
 *	Pick something out of a pack for a purpose
 */
Item *
get_item(const char *purpose, ItemFilter type)
{
	Item *obj;
	unsigned char ch;
	unsigned char och;
	rogue::Turn &turn = game().turn;	//@ lch and wasthing were statics here
	unsigned char gi_state;	/* get item sub state */
	int once_only = FALSE;

	if (((!strncmp(game().options.menu,"sel",3) && strcmp(purpose,"eat")
	  && strcmp(purpose,"drop"))) || !strcmp(game().options.menu,"on"))
		once_only = TRUE;

	gi_state = game().turn.again;
	if (pack.empty())
		msg("you aren't carrying anything");
	else {
		ch = turn.last_item_key;
		for (;;) {
			/*
			 * if we are doing something AGAIN, and the pack hasn't
			 * changed then don't ask just give him the same thing
			 * he got on the last command.
			 */
			if (gi_state && turn.last_item == pack_obj(ch, &och))
				goto skip;
			if (once_only) {
				ch = '*';
				goto skip;
			}
			if (!game().options.brief())
				addmsg("which object do you want to ");
			msg("%s? (* for list): ",purpose);
			/*
			 * ignore any alt characters that may be typed
			 */
			ch = readchar();
			skip:
			game().message.end = 0;
			gi_state = FALSE;
			once_only = FALSE;
			if (ch == '*') {
				if ((ch = inventory(pack, type, purpose)) == 0) {
					game().turn.after = FALSE;
					return NULL;
				}
				if (ch == ' ')
					continue;
				turn.last_item_key = ch;
			}
			/*
			 * Give the poor player a chance to abort the command
			 */
			if (ch == ESCAPE) {
				game().turn.after = FALSE;
				msg("");
				return NULL;
			}
			if ((obj = pack_obj(ch, &och)) == NULL) {
				ifterse1("range is 'a' to '%c'","please specify a letter between 'a' and '%c'", och-1);
				continue;
			} else {
				/*
				 * If you find an object reset flag because
				 * you really don't know if the object he is getting
				 * is going to change the pack.  If he detaches the
				 * thing from the pack later this flag will get set.
				 */
				if (strcmp(purpose, "identify")) {
					turn.last_item_key = ch;
					turn.last_item = obj;
				}
				return obj;
		   }
		}
	}
	return NULL;
}

/*
 * pack_char:
 *	Return which character would address a pack object
 */
unsigned char
pack_char(Item *obj)
{
	Item *item;
	unsigned char c;

	c = 'a';
	for (item = pack.first(); item != NULL; item = pack.after(item))
		if (item == obj)
			return c;
		else
			c++;
	return '?';
}

/*
 * money:
 *	Add or subtract gold from the pack
 */
void
money(int value)
{
	unsigned char floor;

	floor = proom->r_flags.test(RoomFlag::Gone) ? PASSAGE : FLOOR;
	game().player.purse += value;
	display().draw_tile(hero, floor);
	chat(hero.y, hero.x) = floor;
	if (value > 0)
	{
		msg("you found %d gold pieces", value);
	}
}

/*
 * drop:
 *	Put something down
 */
void
drop(void)
{
	unsigned char ch;
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
	msg("dropped %s", inv_name(op, TRUE).c_str());
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
			extinguish(Event::Unsee);
			break;
		}
	}
	return TRUE;
}

}  // namespace rogue::items
