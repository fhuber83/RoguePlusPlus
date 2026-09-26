
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

	if (pack.empty()) {
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
		items.s_know[obj->o_which] = TRUE;
		*items.s_guess[obj->o_which] = '\0';
		break;
	case ItemKind::Potion:
		items.p_know[obj->o_which] = TRUE;
		*items.p_guess[obj->o_which] = '\0';
		break;
	case ItemKind::Stick:
		items.ws_know[obj->o_which] = TRUE;
		obj->o_flags.set(ISKNOW);
		*items.ws_guess[obj->o_which] = '\0';
		break;
	case ItemKind::Weapon:
	case ItemKind::Armor:
		obj->o_flags.set(ISKNOW);
		break;
	case ItemKind::Ring:
		items.r_know[obj->o_which] = TRUE;
		obj->o_flags.set(ISKNOW);
		*items.r_guess[obj->o_which] = '\0';
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

	display().draw_tile(hero, chat(hero.y, hero.x));
	do
	{
		rm = rnd_room();
		rnd_pos(&game().level.rooms[rm], &c);
	} while (!(step_ok(winat(c.y, c.x))));
	if (&game().level.rooms[rm] != proom)
	{
		leave_room(&hero);
		bcopy(hero,c);
		enter_room(&hero);
	}
	else
	{
		bcopy(hero,c);
		look(TRUE);
	}
	display().draw_tile(hero, PLAYER);
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

