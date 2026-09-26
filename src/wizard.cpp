
/*
 * Special wizard commands (some of which are also non-wizard commands
 * under strange circumstances)
 *
 * wizard.c	1.4 (AI Design)	12/14/84
 */

#include "rogue.h"

/*
 * whatis:
 *	What a certin object is
 */
void
whatis(void)
{
	Item *obj;
	rogue::Items &items = game().items;

	if (game().player.body.t_pack.empty()) {
		msg("You don't have anything in your pack to identify");
		return;
	}

	for (;;) {
		if ((obj = get_item("identify", ItemFilter::all())) == NULL) {
			msg("You must identify something");
			msg(" ");
			game().message.end = 0;
		} else
			break;
	}

	switch (obj->o_type) {
	case ItemKind::Scroll:
		items.s_know[obj->which<Scroll>()] = TRUE;
		*items.s_guess[obj->which<Scroll>()] = '\0';
		break;
	case ItemKind::Potion:
		items.p_know[obj->which<Potion>()] = TRUE;
		*items.p_guess[obj->which<Potion>()] = '\0';
		break;
	case ItemKind::Stick:
		items.ws_know[obj->which<Stick>()] = TRUE;
		obj->o_flags.set(ISKNOW);
		*items.ws_guess[obj->which<Stick>()] = '\0';
		break;
	case ItemKind::Weapon:
	case ItemKind::Armor:
		obj->o_flags.set(ISKNOW);
		break;
	case ItemKind::Ring:
		items.r_know[obj->which<Ring>()] = TRUE;
		obj->o_flags.set(ISKNOW);
		*items.r_guess[obj->which<Ring>()] = '\0';
		break;
	default:	// the other kinds of item: nothing
		break;
	}
	/*
	 * If it is vorpally enchanted, then reveal what type of monster it is
	 * vorpally enchanted against
	 */
	if (obj->o_enemy)
		obj->o_flags.set(ISREVEAL);
	msg("{}", inv_name(obj, FALSE));
}


/*
 * telport:
 *	Bamf the hero someplace else
 */
int
teleport(void)
{
	int rm;
	coord c;
	rogue::Player &player = game().player;

	display().draw_tile(player.body.t_pos, game().level.at(player.body.t_pos));
	do
	{
		rm = rnd_room();
		rnd_pos(&game().level.rooms[rm], &c);
	} while (!(step_ok(winat(c.y, c.x))));
	if (&game().level.rooms[rm] != player.body.t_room)
	{
		leave_room(&player.body.t_pos);
		bcopy(player.body.t_pos,c);
		enter_room(&player.body.t_pos);
	}
	else
	{
		bcopy(player.body.t_pos,c);
		look(TRUE);
	}
	display().draw_tile(player.body.t_pos, PLAYER);
	/*
	 * turn off ISHELD in case teleportation was done while fighting
	 * a Fungi
	 */
	if (player.body.t_flags.test(ISHELD)) {
		player.body.t_flags.unset(ISHELD);
		f_restor();
	}
	player.no_move = 0;
	game().turn.count = 0;
	game().turn.running = FALSE;
	flush_type();
	/*
	 * Teleportation can be a confusing experience
	 */
	if (player.body.t_flags.test(ISHUH))
		lengthen(Event::Unconfuse, rnd(4)+2);
	else
		fuse(Event::Unconfuse, rnd(4)+2);
	player.body.t_flags.set(ISHUH);
	return rm;
}

