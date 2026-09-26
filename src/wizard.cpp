
/*
 * Special wizard commands (some of which are also non-wizard commands
 * under strange circumstances)
 *
 * wizard.c	1.4 (AI Design)	12/14/84
 */

#include "rogue.h"

#ifdef WIZARD
static int	get_num(int *place);
#endif


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
	default:	//@ the other kinds of item: nothing
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

#ifdef WIZARD
/*
 * create_obj:
 *	Wizard command for getting anything he wants
 */
void
create_obj(void)
{
	Item *obj;
	unsigned char ch, bless;

	if ((obj = new_item()) == NULL)
	{
		msg("can't create anything now");
		return;
	}
	msg("type of item: ");
	switch (readchar()) {
		case '!': obj->o_type = ItemKind::Potion; break;
		case '?': obj->o_type = ItemKind::Scroll; break;
		case '/': obj->o_type = ItemKind::Stick; break;
		case '=': obj->o_type = ItemKind::Ring; break;
		case ')': obj->o_type = ItemKind::Weapon; break;
		case ']': obj->o_type = ItemKind::Armor; break;
		case ',': obj->o_type = ItemKind::Amulet; break;
		default:
			obj->o_type = ItemKind::Food;
	}
	game().message.end = 0;
	msg("which {:c} do you want? (0-f)", glyph_of(obj->o_type));
	obj->o_which = (is_digit((ch = readchar())) ? ch - '0' : ch - 'a' + 10);
	obj->o_group = 0;
	obj->o_count = 1;
	obj->o_damage = obj->o_hurldmg = "0d0";
	game().message.end = 0;
	if (obj->o_type == ItemKind::Weapon || obj->o_type == ItemKind::Armor)
	{
		msg("blessing? (+,-,n)");
		bless = readchar();
		game().message.end = 0;
		if (bless == '-')
			obj->o_flags.set(ISCURSED);
		if (obj->o_type == ItemKind::Weapon)
		{
			init_weapon(obj, obj->o_which);
			if (bless == '-')
				obj->o_hplus -= rnd(3)+1;
			if (bless == '+')
				obj->o_hplus += rnd(3)+1;
		}
		else
		{
			obj->o_ac = a_class[obj->o_which];
			if (bless == '-')
				obj->o_ac += rnd(3)+1;
			if (bless == '+')
				obj->o_ac -= rnd(3)+1;
		}
	}
	else if (obj->o_type == ItemKind::Ring)
		switch (obj->o_which)
		{
		case R_PROTECT:
		case R_ADDSTR:
		case R_ADDHIT:
		case R_ADDDAM:
			msg("blessing? (+,-,n)");
			bless = readchar();
			game().message.end = 0;
			if (bless == '-')
				obj->o_flags.set(ISCURSED);
			obj->o_ac = (bless == '-' ? -1 : rnd(2) + 1);
			break;
		case R_AGGR:
		case R_TELEPORT:
			obj->o_flags.set(ISCURSED);
			/* fallthrough */
		}
	else if (obj->o_type == ItemKind::Stick)
		fix_stick(obj);
	else if (obj->o_type == ItemKind::Gold)
	{
		msg("how much?");
		get_num(&obj->o_goldval, stdscr);
	}
	add_pack(obj, FALSE);
}
#endif

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
	 * (unless you really are a wizard)
	 */
#ifdef WIZARD
	if (!wizard)
	{
#endif //WIZARD
	if (player.body.t_flags.test(ISHUH))
		lengthen(Event::Unconfuse, rnd(4)+2);
	else
		fuse(Event::Unconfuse, rnd(4)+2);
	player.body.t_flags.set(ISHUH);
#ifdef WIZARD
	}
#endif //WIZARD
	return rm;
}

#ifdef WIZARD

/*
 * show_map:
 *	Print out the map for the wizard
 *	@unused, which is a shame...
 */
static
void
show_map(void)
{
	int y, x, real;

	display().open_page();
	display().clear_page();
	for (y = 1; y < maxrow; y++)
	for (x = 0; x < COLS; x++)
	{
		real = flat(y, x) & F_REAL;
		display().draw_tile({x, y}, chat(y, x), real ? TileStyle::Normal : TileStyle::Inverse);
	}
	show_win("---More (level map)---");
	display().close_page();
}

static
int
get_num(int *place)
{
	char numbuf[12];

	input().read_line(numbuf,10);
	*place = atoi(numbuf);
	return(*place);
}
#endif  // WIZARD
