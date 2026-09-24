/*
 * Hero movement commands
 *
 * move.c	1.4 (A.I. Design)	12/22/84
 */

#include "rogue.h"

/*
 * Used to hold the new hero position
 */
static coord nh;

static byte	be_trapped(coord *tc);

/*
 * do_run:
 *	Start the hero running
 */
void
do_run(byte ch)
{
	game().turn.running = TRUE;
	game().turn.after = FALSE;
	game().turn.run_dir = ch;
}

/*
 * do_move:
 *	Check to see that a move is legal.  If it is handle the
 * consequences (fighting, picking up, etc.)
 */
void
do_move(int dy, int dx)
{
	byte ch;
	int fl;
	rogue::Turn &turn = game().turn;
	rogue::Player &player = game().player;

	turn.first_move = FALSE;
	if (turn.bailout) {
		turn.bailout = FALSE;
		msg("the crack widens ... ");
		descend("");
		return ;
	}
	if (player.no_move) {
		player.no_move--;
		msg("you are still stuck in the bear trap");
		return;
	}
	/*
	 * Do a confused move (maybe)
	 */
	if (on(player.body, ISHUH) && rnd(5) != 0)
		rndmove(&player.body,&nh);
	else {
over:
		nh.y = hero.y + dy;
		nh.x = hero.x + dx;
	}

	/*
	 * Check if he tried to move off the screen or make an illegal
	 * diagonal move, and stop him if he did.
	 * fudge it for 40/80 jll -- 2/7/84
	 */
	if (offmap(nh.y, nh.x))
		goto hit_bound;
	if (!diag_ok(&hero, &nh)) {
		turn.after = FALSE;
		turn.running = FALSE;
		return;
	}
	/*
	 * If you are running and the move does
	 * not get you anywhere stop running
	 */
	if (turn.running && (hero == nh))
		turn.after = turn.running = FALSE;
	fl = flat(nh.y, nh.x);
	ch = winat(nh.y, nh.x);
	/*
	 * When the hero is on the door do not allow him
	 * to run until he enters the room all the way
	 */
	if ((chat(hero.y,hero.x) == DOOR) && (ch == FLOOR))
		turn.running = FALSE;
	if (!(fl & F_REAL) && ch == FLOOR) {
		chat(nh.y, nh.x) = ch = TRAP;
		flat(nh.y, nh.x) |= F_REAL;
	}
	else if (on(player.body, ISHELD) && ch != 'F') {
		msg("you are being held");
		return;
	}
	switch (ch) {
	case ' ':
	case VWALL:
	case HWALL:
	case ULWALL:
	case URWALL:
	case LLWALL:
	case LRWALL:
hit_bound:
		if (turn.running && isgone(proom) && !on(player.body, ISBLIND)) {
			bool	b1, b2;

			switch (turn.run_dir)
			{
			case 'h':
			case 'l':
				b1 = (hero.y > 1 &&
					((flat(hero.y - 1, hero.x) & F_PASS) ||
					  chat(hero.y - 1, hero.x) == DOOR));
				b2 = (hero.y < maxrow - 1 &&
					((flat(hero.y + 1, hero.x) & F_PASS) ||
					  chat(hero.y + 1, hero.x) == DOOR));
				if (!(b1 ^ b2))
					break;
				if (b1) {
					turn.run_dir = 'k';
					dy = -1;
				} else {
					turn.run_dir = 'j';
					dy = 1;
				}
				dx = 0;
				goto over;
			case 'j':
			case 'k':
				b1 = (hero.x > 1 &&
					((flat(hero.y, hero.x - 1) & F_PASS)
					|| chat(hero.y, hero.x - 1) == DOOR));
				b2 = (hero.x < COLS-2 &&
					((flat(hero.y, hero.x + 1) & F_PASS)
					|| chat(hero.y, hero.x + 1) == DOOR));
				if (!(b1 ^ b2))
					break;
				if (b1) {
					turn.run_dir = 'h';
					dx = -1;
				} else {
					turn.run_dir = 'l';
					dx = 1;
				}
				dy = 0;
				goto over;
			}
		}
		turn.after = turn.running = FALSE;
		break;
	case DOOR:
		turn.running = FALSE;
		if (flat(hero.y, hero.x) & F_PASS)
			enter_room(&nh);
		goto move_stuff;
	case TRAP:
		ch = be_trapped(&nh);
		if (ch == T_DOOR || ch == T_TELEP)
			return;
		/* fallthrough */
	case PASSAGE:
		goto move_stuff;
	case FLOOR:
		if (!(fl & F_REAL))
			be_trapped(&hero);
		goto move_stuff;
	default:
		turn.running = FALSE;
		if (ismonster(ch) || moat(nh.y, nh.x))
			fight(&nh, ch, player.weapon, FALSE);
		else {
			turn.running = FALSE;
			if (ch != STAIRS)
				turn.take = ch;
move_stuff:
			display().draw_tile(hero, chat(hero.y, hero.x));
			if ((fl & F_PASS) && (chat(player.old_pos.y, player.old_pos.x) == DOOR
					|| (flat(player.old_pos.y, player.old_pos.x) & F_MAZE)))
				leave_room(&nh);
			if ((fl & F_MAZE) && (flat(player.old_pos.y, player.old_pos.x) & F_MAZE) == 0)
				enter_room(&nh);
			bcopy(hero,nh);
		}
		break;
	}
}

/*
 * door_open:
 *	Called to illuminate a room.  If it is dark, remove anything
 *	that might move.
 */
void
door_open(struct room *rp)
{
	int j, k;
	byte ch;
	THING *item;

	if (!rp->r_flags.test(RoomFlag::Gone) && !on(game().player.body, ISBLIND))
		for (j = rp->r_pos.y; j < rp->r_pos.y + rp->r_max.y; j++)
			for (k = rp->r_pos.x; k < rp->r_pos.x + rp->r_max.x; k++) {
				ch = winat(j, k);
				/* move(j, k); Why do this,?????? */
				if (ismonster(ch)) {
					item = wake_monster(j, k);
					//@ this sanity check was not in original
					if (item == NULL)
					{
						continue;
					}
					if (item->t_oldch == ' ' && !rp->r_flags.test(RoomFlag::Dark)
						&& !on(game().player.body, ISBLIND))
							item->t_oldch = chat(j, k);
				}
			}
}

/*
 * be_trapped:
 *	The guy stepped on a trap.... Make him pay.
 */
static
byte
be_trapped(coord *tc)
{
	byte tr;
	int index;
	rogue::Player &player = game().player;

	game().turn.count = game().turn.running = FALSE;
	index = INDEX(tc->y, tc->x);
	_level[index] = TRAP;
	tr = _flags[index] & F_TMASK;
	player.was_trapped = TRUE;
	switch (tr) {
	when T_DOOR:
		descend("you fell into a trap!");
	when T_BEAR:
		player.no_move += BEARTIME;
		msg("you are caught in a bear trap");
	when T_SLEEP:
		player.no_command += SLEEPTIME;
		player.body.t_flags &= ~ISRUN;
		msg("a %smist envelops you and you fall asleep",
			noterse("strange white "));
	when T_ARROW:
		if (swing(pstats.s_lvl-1, pstats.s_arm, 1)) {
			pstats.s_hpt -= roll(1, 6);
			if (pstats.s_hpt <= 0) {
				msg("an arrow killed you");
				death('a');
			} else
				msg("oh no! An arrow shot you");
		}
		else {
			THING *arrow;

			if ((arrow = new_item()) != NULL) {
				arrow->o_type = WEAPON;
				arrow->o_which = ARROW;
				init_weapon(arrow, ARROW);
				arrow->o_count = 1;
				bcopy(arrow->o_pos,hero);
				fall(arrow, FALSE);
			}
			msg("an arrow shoots past you");
		}
	when T_TELEP:
		teleport();
		display().draw_tile(*tc, TRAP); /* since the hero's leaving, look()
						won't put it on for us */
		/*@
		 * I guess this increment is used solely to signal look() at move.c
		 * about the teleport trap. However, since this increment violates
		 * boolean logic conventions, `was_trapped++` had to be reverted the
		 * real type that bool was typdef'd to in original code: unsigned char.
		 * Either this or refactor the original detection for teleport traps.
		 */
		player.was_trapped++;
	when T_DART:
		if (swing(pstats.s_lvl+1, pstats.s_arm, 1)) {
			pstats.s_hpt -= roll(1, 4);
			if (pstats.s_hpt <= 0) {
				msg("a poisoned dart killed you");
				death('d');
			}
			if (!ISWEARING(R_SUSTSTR) && !save(VS_POISON))
				chg_str(-1);
			msg("a dart just hit you in the shoulder");
		} else
			msg("a dart whizzes by your ear and vanishes");
		break;
	}
	flush_type();
	return tr;
}

void
descend(const char *mesg)
{
	level++;
	if (*mesg == 0)
		msg(" ");
	new_level();
	msg("");
	msg(mesg);
	if (!save(VS_LUCK)) {
		msg("you are damaged by the fall");
		if ((pstats.s_hpt -= roll(1,8)) <= 0)
			death('f');
	}
}

/*
 * rndmove:
 *	Move in a random direction if the monster/person is confused
 */
void
rndmove(THING *who, coord *newmv)
{
	int x, y;
	byte ch;
	THING *obj;

	y = newmv->y = who->t_pos.y + rnd(3) - 1;
	x = newmv->x = who->t_pos.x + rnd(3) - 1;
	/*
	 * Now check to see if that's a legal move.  If not, don't move.
	 * (I.e., bump into the wall or whatever)
	 */
	if (y == who->t_pos.y && x == who->t_pos.x)
		return;
	if ((y < 1 || y >= maxrow) || (x < 0 || x >= COLS))
		goto bad;
	else if (!diag_ok(&who->t_pos, newmv))
		goto bad;
	else {
		ch = winat(y, x);
		if (!step_ok(ch))
			goto bad;
		if (ch == SCROLL) {
			for (obj = lvl_obj; obj != NULL; obj = next(obj))
				if (y == obj->o_pos.y && x == obj->o_pos.x)
					break;
			if (obj != NULL && obj->o_which == S_SCARE)
				goto bad;
		}
	}
	return;

bad:
	bcopy((*newmv),who->t_pos);
	return;
}
