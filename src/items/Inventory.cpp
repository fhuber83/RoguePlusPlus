#include "rogue.h"

namespace rogue::items {

namespace {

/*
 * pack_obj:
 *	The item in the pack with the letter ch, if any
 */
Maybe<Item>
pack_obj(unsigned char ch)
{
	Maybe<Item> obj;
	unsigned char och;
	rogue::Player &player = game().player;

	for (obj = player.body.t_pack.first(), och = 'a'; obj; obj = player.body.t_pack.after(*obj), och++)
		if (ch == och)
			return obj;
	return std::nullopt;
}

/*
 * picked_up:
 *	The rogue has obj in the pack now (was add_pack()'s label): a monster
 *	that wanted it runs at him instead, and he is told.
 */
void
picked_up(Item &obj, bool silent)
{
	Maybe<Creature> mp;
	rogue::Player &player = game().player;
	rogue::Level &level = game().level;

	/*
	 * If this was the object of something's desire, that monster will
	 * get mad and run at the hero
	 */
	for (mp = level.monsters.first(); mp; mp = level.monsters.after(*mp))
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
		/*
		 * Another bug in Rogue: missed null check for t_dest. Monsters could
		 * be not chasing (sleeping, another room, Ice Monster, etc), so a
		 * destination could possibly have never been assigned.
		 */
		if (mp->t_dest && game().where(*mp->t_dest) == obj.o_pos)
			mp->t_dest = Hero{};
	}

	if (obj.o_type == ItemKind::Amulet)
	{
		player.has_amulet = true;
		player.saw_amulet = true;
	}
	/*
	 * Notify the user
	 */
	if (!silent)
		msg("{}{} ({:c})",noterse("you now have "),
			inv_name(obj, true), pack_char(obj));
}

}  // namespace

/*
 * add_pack:
 *	Pick up an object and add it to the pack.  If an item is given, add
 *	it instead of getting it off the ground.
 */
void
add_pack(Maybe<Item> given, bool silent)
{
	Maybe<Item> obj, op, lp;
	bool exact, from_floor;
	unsigned char floor;
	rogue::Player &player = game().player;
	rogue::Level &level = game().level;

	if (!given)
	{
		from_floor = true;
		if (!(obj = find_obj(player.body.t_pos.y, player.body.t_pos.x)))
			return;
	}
	else
	{
		from_floor = false;
		obj = given;
	}
	/*
	 * Link it into the pack.  Search the pack for a object of similar type
	 * if there isn't one, stuff it at the beginning, if there is, look for one
	 * that is exactly the same and just increment the count if there is.
	 * Food is always put at the beginning for ease of access, but it
	 * is not ordered so that you can't tell good food from bad.  First check
	 * to see if there is something in the same group and if there is then
	 * increment the count.
	 */

	/*
	 *  bug in original Rogue: it didn't check that the rogue's room (t_room)
	 *  is not null, as is the case when add_pack() is called from
	 *  init_player(), which happens before any room even exist. t_room is
	 *  set in enter_room(), which is first called in new_level()
	 */
	floor = (player.body.t_room && game().level.room(*player.body.t_room).r_flags.test(RoomFlag::Gone)) ? PASSAGE : FLOOR;
	if (obj->o_group)
	{
		for (op = player.body.t_pack.first(); op; op = player.body.t_pack.after(*op))
		{
			if (op->o_group == obj->o_group)
			{
			/*
			 * Put it in the pack and notify the user
			 */
				op->o_count += obj->o_count;
				if (from_floor)
				{
					level.objects.remove(*obj);
					display().draw_tile(player.body.t_pos, floor);
					level.at(player.body.t_pos) = floor;
				}
				discard(*obj);
				picked_up(*op, silent);
				return;
			}
		}
	}
	/*
	 * Check if there is room
	 */
	if (player.in_pack >= MAXPACK-1)
	{
		msg("you can't carry anything else");
		return;
	}
	/*
	 * Check for and deal with scare monster scrolls
	 */
	if (obj->o_type == ItemKind::Scroll && obj->which<Scroll>() == Scroll::ScareMonster)
	{
		if (obj->o_flags.test(rogue::ItemFlag::Found))
		{
			level.objects.remove(*obj);
			display().draw_tile(player.body.t_pos, floor);
			level.at(player.body.t_pos) = floor;
			msg("the scroll turns to dust{}.", noterse(" as you pick it up"));
			return;
		}
		else
			obj->o_flags.set(rogue::ItemFlag::Found);
	}

	player.in_pack++;
	if (from_floor)
	{
		level.objects.remove(*obj);
		display().draw_tile(player.body.t_pos, floor);
		level.at(player.body.t_pos) = floor;
	}
	/*
	 * Search for an object of the same type
	 */
	exact = false;
	for (op = player.body.t_pack.first(); op; op = player.body.t_pack.after(*op))
		if (obj->o_type == op->o_type)
			break;
	if (!op)
	{
		/*
		 * Put it at the end of the pack since it is a new type
		 */
		for (op = player.body.t_pack.first(); op; op = player.body.t_pack.after(*op))
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
				exact = true;
				break;
			}
			lp = op;
			if (!(op = player.body.t_pack.after(*op)))
				break;
		}
	}
	if (!op)
	{
		/*
		 * Didn't find an exact match, just stick it here
		 */
		player.body.t_pack.insert_after(lp, *obj);	// lp is null only when the pack is empty
	}
	else
	{
		/*
		 * If we found an exact match.  If it is a potion, food, or a
		 * scroll, increase the count, otherwise put it with its clones.
		 */
		if (exact && is_multiple(obj->o_type))
		{
			op->o_count++;
			discard(*obj);
			picked_up(*op, silent);
			return;
		}
		player.body.t_pack.insert_before(*op, *obj);
	}
	picked_up(*obj, silent);
}

/*
 * inventory:
 *	List what is in the pack
 */
unsigned char
inventory(const List<Item> &list, ItemFilter type, std::string_view lstr)
{
	unsigned char ch;
	Maybe<Item> obj;
	int n_objs;

	n_objs = 0;
	for (ch = 'a', obj = list.first(); obj; ch++, obj = list.after(*obj))
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
		  !(type.is(ItemKind::Stick) && obj->o_enemy && obj->charges()))
			continue;
		n_objs++;
		add_line(lstr, std::format("{}) {}", static_cast<char>(ch), inv_name(*obj, false)));
	}
	if (n_objs == 0)
	{
		msg("{}", type.is_all() ? "you are empty handed" :
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
	Maybe<Item> obj;
	rogue::Player &player = game().player;

	switch (ch)
	{
	case GOLD:
	{
		Maybe<Creature> mp;

		if (!(obj = find_obj(player.body.t_pos.y, player.body.t_pos.x)))
		return;
		money(obj->gold_value());
		/*
		 * find_dest() can point a monster's t_dest straight at this gold's
		 * o_pos. Redirect it to the hero before the gold's pool slot is
		 * discarded, same as add_pack()'s picked_up() redirect for other
		 * floor items, so nothing is left pointing at a freed Item.
		 */
		for (mp = game().level.monsters.first(); mp; mp = game().level.monsters.after(*mp))
			if (mp->t_dest && game().where(*mp->t_dest) == obj->o_pos)
				mp->t_dest = Hero{};
		game().level.objects.remove(*obj);
		discard(*obj);
		game().level.room(*player.body.t_room).r_goldval = 0;
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
		add_pack(std::nullopt, false);
		break;
	}
}

/*
 * get_item:
 *	Pick something out of a pack for a purpose
 */
Maybe<Item>
get_item(std::string_view purpose, ItemFilter type)
{
	Maybe<Item> obj;
	unsigned char ch;
	rogue::Turn &turn = game().turn;
	unsigned char gi_state;	/* get item sub state */
	bool once_only = false;

	if (((game().options.menu.starts_with("sel") && purpose != "eat"
	  && purpose != "drop")) || game().options.menu == "on")
		once_only = true;

	gi_state = game().turn.again;
	if (game().player.body.t_pack.empty())
		msg("you aren't carrying anything");
	else {
		ch = turn.last_item_key;
		for (;;) {
			/*
			 * if we are doing something AGAIN, and the pack hasn't
			 * changed then don't ask just give him the same thing
			 * he got on the last command.
			 */
			if (!(gi_state && game().pool.item(turn.last_item) == pack_obj(ch))) {
				if (once_only)
					ch = '*';
				else {
					if (!game().options.brief())
						addmsg("which object do you want to ");
					msg("{}? (* for list): ",purpose);
					/*
					 * ignore any alt characters that may be typed
					 */
					ch = readchar();
				}
			}
			game().message.end = 0;
			gi_state = false;
			once_only = false;
			if (ch == '*') {
				if ((ch = inventory(game().player.body.t_pack, type, purpose)) == 0) {
					game().turn.after = false;
					return std::nullopt;
				}
				if (ch == ' ')
					continue;
				turn.last_item_key = ch;
			}
			/*
			 * Give the poor player a chance to abort the command
			 */
			if (ch == ESCAPE) {
				game().turn.after = false;
				msg("");
				return std::nullopt;
			}
			if (!(obj = pack_obj(ch))) {
				int last = 'a' + static_cast<int>(game().player.body.t_pack.size()) - 1;
				ifterse("range is 'a' to '{:c}'","please specify a letter between 'a' and '{:c}'", last);
				continue;
			} else {
				/*
				 * If you find an object reset flag because
				 * you really don't know if the object he is getting
				 * is going to change the pack.  If he detaches the
				 * thing from the pack later this flag will get set.
				 */
				if (purpose != "identify") {
					turn.last_item_key = ch;
					turn.last_item = game().pool.id_of(obj);
				}
				return obj;
		   }
		}
	}
	return std::nullopt;
}

/*
 * pack_char:
 *	Return which character would address a pack object
 */
unsigned char
pack_char(const Item &obj)
{
	Maybe<Item> item;
	unsigned char c;
	rogue::Player &player = game().player;

	c = 'a';
	for (item = player.body.t_pack.first(); item; item = player.body.t_pack.after(*item))
		if (refers_to(item, obj))
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
	rogue::Player &player = game().player;

	floor = game().level.room(*player.body.t_room).r_flags.test(RoomFlag::Gone) ? PASSAGE : FLOOR;
	player.purse += value;
	display().draw_tile(player.body.t_pos, floor);
	game().level.at(player.body.t_pos) = floor;
	if (value > 0)
	{
		msg("you found {} gold pieces", value);
	}
}

/*
 * drop:
 *	Put something down
 */
void
drop()
{
	unsigned char ch;
	Maybe<Item> nobj, op;
	rogue::Player &player = game().player;

	ch = game().level.at(player.body.t_pos);
	if (ch != FLOOR && ch != PASSAGE)
	{
		msg("there is something there already");
		return;
	}
	if (!(op = get_item("drop", ItemFilter::all())))
		return;
	if (!can_drop(*op))
		return;
	/*
	 * Take it out of the pack
	 */
	if (op->o_count >= 2 && op->o_type != ItemKind::Weapon)
	{
		if (!(nobj = new_item()))
		{
			msg("{}it appears to be stuck in your pack!",
				noterse("can't drop it, "));
			return;
		}
		op->o_count--;
		*nobj = *op;
		nobj->o_count = 1;
		op = nobj;
		if (op->o_group != 0)
			player.in_pack++;
	}
	else
		player.body.t_pack.remove(*op);
	player.in_pack--;
	/*
	 * Link it into the level object list
	 */
	game().level.objects.push_front(*op);
	game().level.at(player.body.t_pos) = glyph_of(op->o_type);
	op->o_pos = player.body.t_pos;
	if (op->o_type == ItemKind::Amulet)
		player.has_amulet = false;
	msg("dropped {}", inv_name(*op, true));
}

/*
 * can_drop:
 *	Do special checks for dropping or unweilding|unwearing|unringing
 */
bool
can_drop(const Item &op)
{
	rogue::Player &player = game().player;
	if (!refers_to(player.armor_item(), op) && !refers_to(player.weapon_item(), op)
		&& !refers_to(player.ring_item(Hand::Left), op) && !refers_to(player.ring_item(Hand::Right), op))
		return true;
	if (op.o_flags.test(ISCURSED)) {
		msg("you can't.  It appears to be cursed");
		return false;
	}
	if (refers_to(player.weapon_item(), op))
		player.weapon = std::nullopt;
	else if (refers_to(player.armor_item(), op)) {
		waste_time();
		player.armor = std::nullopt;
	} else {
		Hand hand;

		if (!refers_to(player.ring_item(hand = Hand::Left), op))
			if (!refers_to(player.ring_item(hand = Hand::Right), op)) {
				if constexpr (rogue::config::debug_checks)
					debug("Candrop called with funny thing");
				return true;
			}
		player.rings[hand] = std::nullopt;
		switch (op.which<Ring>()) {
		case Ring::AddStrength:
			chg_str(-op.o_ac);
			break;
		case Ring::SeeInvisible:
			unsee();
			extinguish(Event::Unsee);
			break;
		default:
			break;
		}
	}
	return true;
}

/*
 * is_current:
 *	See if the object is one of the currently used items
 */
bool
is_current(const Item &obj)
{
	if (refers_to(game().player.armor_item(), obj) || refers_to(game().player.weapon_item(), obj) || refers_to(game().player.ring_item(Hand::Left), obj)
		|| refers_to(game().player.ring_item(Hand::Right), obj)) {
		msg("That's already in use");
		return true;
	}
	return false;
}

}  // namespace rogue::items
