/*
 * All sorts of miscellaneous routines
 *
 * misc.c	1.4		(A.I. Design)	12/14/84
 */

#include "rogue.h"

/*
 * tr_name:
 *	Print the name of a trap
 */
const char *
tr_name(byte type)
{
	switch (type)
	{
	case T_DOOR:
		return "a trapdoor";
	case T_BEAR:
		return "a beartrap";
	case T_SLEEP:
		return "a sleeping gas trap";
	case T_ARROW:
		return "an arrow trap";
	case T_TELEP:
		return "a teleport trap";
	case T_DART:
		return "a poison dart trap";
	}
	msg("wierd trap: %d", type);
	return NULL;
}

/*
 * look:
 *	A quick glance all around the player
 */
void
look(bool wakeup)
{
	int x, y;
	byte ch, pch;
	int index;
	Creature *tp;
	rogue::Turn &turn = game().turn;
	rogue::Player &player = game().player;
	rogue::Level &level = game().level;
	struct room *rp;
	int ey, ex;
	int passcount = 0;
	byte pfl, *fp;
	int sy, sx, sumhero = 0, diffhero = 0;

	rp = proom;
	index = INDEX(hero.y, hero.x);
	pfl = level.flags[index];
	pch = level.map[index];
	/*
	 * if the hero has moved
	 */
	if (!(player.old_pos == hero)) {
		if (!on(player.body,ISBLIND)) {
			for (x = player.old_pos.x - 1; x <= (player.old_pos.x + 1); x++)
				for (y = player.old_pos.y - 1; y <= (player.old_pos.y + 1); y++) {
					if ((y == hero.y && x == hero.x) || offmap(y,x))
						continue;
					ch = display().tile_at({x, y});
					if (ch == FLOOR) {
						if (player.old_room->r_flags.test(RoomFlag::Dark) && !player.old_room->r_flags.test(RoomFlag::Gone))
							display().draw_tile({x, y}, ' ');
					} else {
						fp = &level.flags[INDEX(y,x)];
						/*
						 * if the maze or passage (that the hero is in!!)
						 * needs to be redrawn (passages once draw always
						 * stay on) do it now.
						 */
						if (((*fp&F_MAZE) || (*fp&F_PASS)) && (ch!=PASSAGE)
							&& (ch != STAIRS) &&
							((*fp & F_PNUM) == (pfl & F_PNUM)) )
								display().draw_tile({x, y}, PASSAGE);
					}
				}
		}
		player.old_pos = hero;
		player.old_room = rp;
	}
	ey = hero.y + 1;
	ex = hero.x + 1;
	sx = hero.x - 1;
	sy = hero.y - 1;
	if (turn.door_stop && !turn.first_move && turn.running) {
		sumhero = hero.y + hero.x;
		diffhero = hero.y - hero.x;
	}
	for (y = sy; y <= ey; y++)
		if (y > 0 && y < maxrow) for (x = sx; x <= ex; x++) {
			if (x <= 0 || x >= COLS)
				continue;
			if (!on(player.body, ISBLIND)) {
				if (y == hero.y && x == hero.x)
					continue;
			} else if (y != hero.y || x != hero.x)
				continue;

			index = INDEX(y, x);
			/*
			 * THIS REPLICATES THE moat() MACRO.  IF MOAT IS CHANGED,
			 * THIS MUST BE CHANGED ALSO ?? What does this really mean ??
			 */
			fp = &level.flags[index];
			ch = level.map[index];
			/*
			 * No Doors
			 */
			if (pch != DOOR && ch != DOOR) {
				/*
				 * Either hero or other in a passage
				 */
				if ((pfl & F_PASS) != (*fp & F_PASS)) {
					/*
					 * Neither is in a maze
					 */
					if ( ! (pfl & F_MAZE) && ! (*fp & F_MAZE))
						continue;
				}
				/*
				 * Not in same passage
				 */
				else if ((*fp & F_PASS) && (*fp & F_PNUM) != (pfl & F_PNUM))
					continue;
			}

			if ((tp = moat(y,x)) != NULL) {
				if (on(player.body, SEEMONST) && on(*tp, ISINVIS)) {
					if (turn.door_stop && !turn.first_move)
						turn.running = FALSE;
					continue;
				} else {
					if (wakeup)
						wake_monster(y, x);
					if (tp->t_oldch != ' ' ||
						(!rp->r_flags.test(RoomFlag::Dark) && !on(player.body, ISBLIND)))
							tp->t_oldch = level.map[index];
					if (see_monst(tp))
						ch = tp->t_disguise;
				}
			}

			/*
			 * The current character used for IBM ARMOR doesn't
			 * look right in Inverse
			 */
			display().draw_tile({x, y}, ch,
					((ch!=PASSAGE) && (*fp & (F_PASS | F_MAZE)) && ch != ARMOR)
						? TileStyle::Inverse : TileStyle::Normal);

			if (turn.door_stop && !turn.first_move && turn.running) {
				switch (turn.run_dir) {
				when 'h':
					if (x == ex)
						continue;
				when 'j':
					if (y == sy)
						continue;
				when 'k':
					if (y == ey)
						continue;
				when 'l':
					if (x == sx)
						continue;
				when 'y':
					if ((y + x) - sumhero >= 1)
						continue;
				when 'u':
					if ((y - x) - diffhero >= 1)
						continue;
				when 'n':
					if ((y + x) - sumhero <= -1)
						continue;
				when 'b':
					if ((y - x) - diffhero <= -1)
						continue;
					break;
				}
				switch (ch) {
				case DOOR:
					if (x == hero.x || y == hero.y)
						turn.running = FALSE;
					break;
				case PASSAGE:
					if (x == hero.x || y == hero.y)
						passcount++;
					break;
				case FLOOR:
				case VWALL:
				case HWALL:
				case ULWALL:
				case URWALL:
				case LLWALL:
				case LRWALL:
				case ' ':
					break;
				default:
					turn.running = FALSE;
					break;
				}
			}
		}
	if (turn.door_stop && !turn.first_move && passcount > 1)
		turn.running = FALSE;
	/*@
	 * The expression (was_trapped > TRUE) would never evaluate to true if
	 * `was_trapped` was a real boolean. I guess this is specifically testing
	 * for the `was_trapped++` case in be_trapped() at move.c, triggered by
	 * a teletransporting trap.
	 * Not an issue in the original code, as bool was typedef'd to unsigned char.
	 * This test helped reverting `was_trapped` to an unsigned char. However,
	 * I guess int would be a better type, or perhape another logic to detect
	 * teleport traps.
	 */
	display().draw_tile(hero, PLAYER,
			((flat(hero.y,hero.x) & F_PASS) || (player.was_trapped > TRUE)
					|| (flat(hero.y,hero.x) & F_MAZE))
				? TileStyle::Inverse : TileStyle::Normal);
	if (player.was_trapped) {
		display().bell();
		player.was_trapped = FALSE;
	}
}

/*
 * find_obj:
 *	Find the unclaimed object at y, x
 */
Item *
find_obj(int y, int x)
{
	Item *op;

	for (op = game().level.objects; op != NULL; op = next(op))
		if (op->o_pos.y == y && op->o_pos.x == x)
			return op;
#ifdef DEBUG
	debug(sprintf(prbuf, "Non-object %c %d,%d", chat(y, x), y, x));
	return NULL;
#else
	/* NOTREACHED */
#endif
	return NULL;
}

/*
 * eat:
 *	She wants to eat something, so let her try
 */
void
eat()
{
	Item *obj;
	rogue::Player &player = game().player;

	if ((obj = get_item("eat", ItemKind::Food)) == NULL)
		return;
	if (obj->o_type != ItemKind::Food)
	{
		msg("ugh, you would get ill if you ate that");
		return;
	}
	player.in_pack--;
	if (--obj->o_count < 1)
	{
		detach(pack, obj);
		discard(obj);
	}
	if (player.food_left < 0)
		player.food_left = 0;
	if (player.food_left > (STOMACHSIZE - 20))
		player.no_command += 2 + rnd(5);
	if ((player.food_left += HUNGERTIME - 200 + rnd(400)) > STOMACHSIZE)
		player.food_left = STOMACHSIZE;
	player.hungry_state = 0;
	if (obj == player.weapon)
		player.weapon = NULL;
	if (obj->o_which == 1)
		msg("my, that was a yummy %s", game().options.fruit);
	else
		if (rnd(100) > 70)
		{
			pstats.s_exp++;
			msg("yuk, this food tastes awful");
			check_level();
		}
		else
			msg("yum, that tasted good");
	if (player.no_command)
		msg("You feel bloated and fall asleep");
}

/*
 * chg_str:
 *	Used to modify the player's strength.  It keeps track of the
 *	highest it has been, just in case
 */
void
chg_str(int amt)
{
	str_t comp;

	if (amt == 0)
	return;
	add_str(&pstats.s_str, amt);
	comp = pstats.s_str;
	if (ISRING(LEFT, R_ADDSTR))
		add_str(&comp, -game().player.rings[LEFT]->o_ac);
	if (ISRING(RIGHT, R_ADDSTR))
		add_str(&comp, -game().player.rings[RIGHT]->o_ac);
	if (comp > game().player.max_stats.s_str)
		game().player.max_stats.s_str = comp;
}

/*
 * add_str:
 *	Perform the actual add, checking upper and lower bound
 */
void
add_str(str_t *sp, int amt)
{
	if ((*sp += amt) < 3)
		*sp = 3;
	else if (*sp > 31)
		*sp = 31;
}

/*
 * add_haste:
 *	Add a haste to the player
 */
bool
add_haste(bool potion)
{
	rogue::Player &player = game().player;
	if (on(player.body, ISHASTE))
	{
		player.no_command += rnd(8);
		player.body.t_flags.unset(ISRUN);
		extinguish(nohaste);
		player.body.t_flags.unset(ISHASTE);
		msg("you faint from exhaustion");
		return FALSE;
	}
	else
	{
		player.body.t_flags.set(ISHASTE);
		if (potion)
			fuse(nohaste, rnd(4)+10);
		return TRUE;
	}
}

/*
 * aggravate:
 *	Aggravate all the monsters on this level
 */
void
aggravate()
{
	Creature *mi;

	for (mi = game().level.monsters; mi != NULL; mi = next(mi))
		start_run(&mi->t_pos);
}

/*
 * vowelstr:
 *      For printfs: if string starts with a vowel, return "n" for an
 *	"an".
 */
const char *
vowelstr(const char *str)
{
	switch (*str)
	{
	case 'a': case 'A':
	case 'e': case 'E':
	case 'i': case 'I':
	case 'o': case 'O':
	case 'u': case 'U':
		return "n";
	default:
		return "";
	}
}

/*
 * is_current:
 *	See if the object is one of the currently used items
 */
bool
is_current(Item *obj)
{
	if (obj == NULL)
		return FALSE;
	if (obj == game().player.armor || obj == game().player.weapon || obj == game().player.rings[LEFT]
		|| obj == game().player.rings[RIGHT]) {
		msg("That's already in use");
		return TRUE;
	}
	return FALSE;
}

/*
 * get_dir:
 *      Set up the direction co_ordinate for use in varios "prefix"
 *	commands
 */
bool
get_dir()
{
	int ch;
	rogue::Turn &turn = game().turn;

	if (turn.again)
		return TRUE;
	msg("which direction? ");
	do
		if ((ch = readchar()) == ESCAPE) {
			msg("");
			return FALSE;
		}
	while (find_dir(ch, &turn.delta) == 0);
	msg("");
	if (on(game().player.body, ISHUH) && rnd(5) == 0)
		do {
			turn.delta.y = rnd(3) - 1;
			turn.delta.x = rnd(3) - 1;
		} while (turn.delta.y == 0 && turn.delta.x == 0);
	return TRUE;
}

bool
find_dir(byte ch, coord *cp)
{
	bool gotit;

	gotit = TRUE;
	switch (ch) {
		when 'h': case'H': cp->y =  0; cp->x = -1;
		when 'j': case'J': cp->y =  1; cp->x =  0;
		when 'k': case'K': cp->y = -1; cp->x =  0;
		when 'l': case'L': cp->y =  0; cp->x =  1;
		when 'y': case'Y': cp->y = -1; cp->x = -1;
		when 'u': case'U': cp->y = -1; cp->x =  1;
		when 'b': case'B': cp->y =  1; cp->x = -1;
		when 'n': case'N': cp->y =  1; cp->x =  1;
		otherwise: gotit = FALSE;
	}
	return gotit;
}

/*
 * sign:
 *	Return the sign of the number
 */
shint
sign(int nm)
{
	if (nm < 0)
		return -1;
	else
		return (nm > 0);
}

/*
 * spread:
 *	Give a spread around a given number (+/- 10%)
 */
int
spread(int nm)
{
	return nm - nm / 10 + rnd(nm / 5);
}

/*
 * call_it:
 *	Call an object something after use.
 */
void
call_it(bool know, char **guess)
{
	if (know && **guess)
		**guess = '\0';
	else if (!know && **guess == '\0') {
		msg("%scall it? ",noterse("what do you want to "));
		input().read_line(prbuf,MAXNAME);
		if (*prbuf != ESCAPE)
			strcpy(*guess, prbuf);
		msg("");
	}
}

/*
 * step_ok:
 *	Returns true if it is ok to step on ch
 */
bool
step_ok(byte ch)
{
	switch (ch)
	{
	case ' ':
	case VWALL:
	case HWALL:
	case ULWALL:
	case URWALL:
	case LLWALL:
	case LRWALL:
		return FALSE;
	default:
		return (!ismonster(ch));
	}
}

/*
 * goodch:
 *	Decide how good an object is and return the correct character for
 * printing.
 */
char
goodch(Item *obj)
{
	char ch = MAGIC;

	if (obj->o_flags.test(ISCURSED))
		ch = BMAGIC;
	switch (obj->o_type) {
	when ItemKind::Armor:
		if (obj->o_ac > a_class[obj->o_which])
			ch = BMAGIC;
	when ItemKind::Weapon:
		if (obj->o_hplus < 0 || obj->o_dplus < 0)
			ch = BMAGIC;
	when ItemKind::Scroll:
		switch (obj->o_which) {
		when S_SLEEP:
		case S_CREATE:
		case S_AGGR:
			ch = BMAGIC;
			break;
		}
	when ItemKind::Potion:
		switch (obj->o_which) {
		when P_CONFUSE:
		case P_PARALYZE:
		case P_POISON:
		case P_BLIND:
			ch = BMAGIC;
			break;
		}
	when ItemKind::Stick:
		switch (obj->o_which) {
		when WS_HASTE_M:
		case WS_TELTO:
			ch = BMAGIC;
			break;
		}
	when ItemKind::Ring:
		switch (obj->o_which) {
		when R_PROTECT:
		case R_ADDSTR:
		case R_ADDDAM:
		case R_ADDHIT:
			if (obj->o_ac < 0)
				ch = BMAGIC;
		when R_AGGR:
		case R_TELEPORT:
			ch = BMAGIC;
			break;
		}
		break;
	otherwise:	//@ the other kinds of item: nothing
		break;
	}
	return ch;
}

/*
 * help: prints out help screens
 */
void
help(struct h_list *helpscr)
{
	int hcount = 0;
	int hrow, hcol;
	int isfull;
	byte answer = 0;

	display().open_page();
	while (*helpscr->h_desc && answer != ESCAPE)
	{
		isfull = FALSE;
		if ((hcount % (game().options.terse?23:46)) == 0)
			display().clear_page();
		/*
		 * determine row and column
		 */
		hcol = 0;
		if (game().options.terse)
		{
			hrow = hcount % 23;
			if (hrow == 22)
				isfull = TRUE;
		}
		else
		{
			hrow = (hcount % 46) / 2;
			if (hcount % 2)
				hcol = 40;
			if (hrow == 22 && hcol == 40)
				 isfull = TRUE;
		}

		display().write_at(hrow, hcol, (const char *)helpscr->h_chstr);
		display().write(helpscr->h_desc);
		helpscr++;

		/*
		 * decide if we need print a continue type message
		 */
		if ( (*helpscr->h_desc == 0) || isfull)
		{
			if (*helpscr->h_desc == 0)
				display().write_at(24, 0, "--press space to continue--");
			else if (game().options.terse)
				display().write_at(24, 0, "--Space for more, Esc to continue--");
			else
				display().write_at(24, 0, "--Press space for more, Esc to continue--");
			do
				answer = readchar();
			while (answer != ' ' && answer != ESCAPE) ;
		}
		hcount++;
	}
	display().close_page();
}


int
DISTANCE(int y1, int x1, int y2, int x2)
{
	return rogue::distance_sq({x1, y1}, {x2, y2});
}

int
INDEX(int y, int x)
{
#ifdef DEBUG
	if (offmap(y,x) && me())
		fatal("BAD INDEX");
#endif //DEBUG
	return((x * (maxrow-1)) + y - 1);
}

bool
offmap(int y, int x)
{
	return (y < 1 || y >= maxrow || x < 0 || x >= COLS) ;
}

byte
winat(int y, int x)
{
	return(moat(y,x) != NULL ? moat(y,x)->t_disguise : chat(y,x));
}

/*
 * search:
 *	Player gropes about him to find hidden things.
 */
void
search()
{
	int y, x;
	byte *fp;
	int ey, ex;

	if (on(game().player.body, ISBLIND))
		return;
	ey = hero.y + 1;
	ex = hero.x + 1;
	for (y = hero.y - 1; y <= ey; y++)
		for (x = hero.x - 1; x <= ex; x++)
		{
			if ((y == hero.y && x == hero.x) || offmap(y, x))
				continue;
			fp = &flat(y, x);
			if (!(*fp & F_REAL))
				switch (chat(y, x))
				{
					case VWALL:
					case HWALL:
					case ULWALL:
					case URWALL:
					case LLWALL:
					case LRWALL:
						if (rnd(5) != 0)
							break;
						chat(y, x) = DOOR;
						*fp |= F_REAL;
						game().turn.count = game().turn.running = FALSE;
						break;
					case FLOOR:
						if (rnd(2) != 0)
							break;
						chat(y, x) = TRAP;
						*fp |= F_REAL;
						game().turn.count = game().turn.running = FALSE;
						msg("you found %s", tr_name(*fp & F_TMASK));
						break;
				}
		}
}


/*
 * d_level:
 *	He wants to go down a level
 */
void
d_level()
{
	if (chat(hero.y, hero.x) != STAIRS)
		msg("I see no way down");
	else {
		game().level.depth++;
		new_level();
	}
}

/*
 * u_level:
 *	He wants to go up a level
 */
void
u_level()
{
	if (chat(hero.y, hero.x) == STAIRS)
		if (game().player.has_amulet) {
			game().level.depth--;
			if (game().level.depth == 0)
				total_winner();
			new_level();
			msg("you feel a wrenching sensation in your gut");
		}
		else
			msg("your way is magically blocked");
	else
		msg("I see no way up");
}

/*
 * call:
 *	Allow a user to call a potion, scroll, or ring something
 */
void
call()
{
	Item *obj;
	char **guess;
	const char *elsewise;
	bool *know;
	rogue::Items &items = game().items;

	obj = get_item("call", ItemFilter::callable());
	/*
	 * Make certain that it is somethings that we want to wear
	 */
	if (obj == NULL)
		return;
	switch (obj->o_type)
	{
	when ItemKind::Ring:
		guess = (char **)items.r_guess;
		know = items.r_know;
		elsewise = (*guess[obj->o_which] != '\0' ?
			guess[obj->o_which] : items.r_stones[obj->o_which]);
	when ItemKind::Potion:
		guess = (char **)items.p_guess;
		know = items.p_know;
		elsewise = (*guess[obj->o_which] != '\0' ?
			guess[obj->o_which] : items.p_colors[obj->o_which]);
	when ItemKind::Scroll:
		guess = (char **)items.s_guess;
		know = items.s_know;
		elsewise = (*guess[obj->o_which] != '\0' ?
			guess[obj->o_which] : items.s_names[obj->o_which].storage);
	when ItemKind::Stick:
		guess = (char **)items.ws_guess;
		know = items.ws_know;
		elsewise = (*guess[obj->o_which] != '\0' ?
			guess[obj->o_which] : items.ws_made[obj->o_which]);
	otherwise:
		msg("you can't call that anything");
		return;
	}
	if (know[obj->o_which])
	{
		msg("that has already been identified");
		return;
	}
	msg("Was called \"%s\"", elsewise);
	msg("what do you want to call it? ");
	input().read_line(prbuf,MAXNAME);
	if (*prbuf && *prbuf != ESCAPE)
		strcpy(guess[obj->o_which], prbuf);
	msg("");
}

/*
 * prompt player for definition of macro
 */
void
do_macro(char *buf, int sz)
{
	char *cp = prbuf;

	msg("F9 was %s, enter new macro: ",buf);
	if (input().read_line(prbuf,sz-1) != ESCAPE)
		do {
			if (*cp != CTRL('F'))
				*buf++ = *cp;
		} while (*cp++) ;
	msg("");
	flush_type();
}



