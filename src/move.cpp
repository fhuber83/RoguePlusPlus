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

static Trap	be_trapped(coord *tc);

/*
 * do_run:
 *	Start the hero running
 */
void
do_run(unsigned char ch)
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
	unsigned char ch;
	Trap trap;
	MapFlags fl;
	rogue::Turn &turn = game().turn;
	rogue::Player &player = game().player;
	rogue::Level &level = game().level;

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
	if (player.body.t_flags.test(ISHUH) && rnd(5) != 0)
		rndmove(&player.body,&nh);
	else {
over:
		nh.y = player.body.t_pos.y + dy;
		nh.x = player.body.t_pos.x + dx;
	}

	/*
	 * Check if he tried to move off the screen or make an illegal
	 * diagonal move, and stop him if he did.
	 * fudge it for 40/80 jll -- 2/7/84
	 */
	if (offmap(nh.y, nh.x))
		goto hit_bound;
	if (!diag_ok(&player.body.t_pos, &nh)) {
		turn.after = FALSE;
		turn.running = FALSE;
		return;
	}
	/*
	 * If you are running and the move does
	 * not get you anywhere stop running
	 */
	if (turn.running && (player.body.t_pos == nh))
		turn.after = turn.running = FALSE;
	fl = level.flags_at(nh);
	ch = winat(nh.y, nh.x);
	/*
	 * When the hero is on the door do not allow him
	 * to run until he enters the room all the way
	 */
	if ((level.at(player.body.t_pos) == DOOR) && (ch == FLOOR))
		turn.running = FALSE;
	if (!fl.test(MapFlag::Real) && ch == FLOOR) {
		level.at(nh) = ch = TRAP;
		level.flags_at(nh).set(MapFlag::Real);
	}
	else if (player.body.t_flags.test(ISHELD) && ch != 'F') {
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
		if (turn.running && player.body.t_room->is_gone() && !player.body.t_flags.test(ISBLIND)) {
			bool	b1, b2;

			switch (turn.run_dir)
			{
			case 'h':
			case 'l':
				b1 = (player.body.t_pos.y > 1 &&
					(level.flags_at(player.body.t_pos.y - 1, player.body.t_pos.x).test(MapFlag::Passage) ||
					  level.at(player.body.t_pos.y - 1, player.body.t_pos.x) == DOOR));
				b2 = (player.body.t_pos.y < maxrow - 1 &&
					(level.flags_at(player.body.t_pos.y + 1, player.body.t_pos.x).test(MapFlag::Passage) ||
					  level.at(player.body.t_pos.y + 1, player.body.t_pos.x) == DOOR));
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
				b1 = (player.body.t_pos.x > 1 &&
					(level.flags_at(player.body.t_pos.y, player.body.t_pos.x - 1).test(MapFlag::Passage)
					|| level.at(player.body.t_pos.y, player.body.t_pos.x - 1) == DOOR));
				b2 = (player.body.t_pos.x < COLS-2 &&
					(level.flags_at(player.body.t_pos.y, player.body.t_pos.x + 1).test(MapFlag::Passage)
					|| level.at(player.body.t_pos.y, player.body.t_pos.x + 1) == DOOR));
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
		if (level.flags_at(player.body.t_pos).test(MapFlag::Passage))
			enter_room(&nh);
		goto move_stuff;
	case TRAP:
		trap = be_trapped(&nh);
		if (trap == Trap::Door || trap == Trap::Teleport)
			return;
		/* fallthrough */
	case PASSAGE:
		goto move_stuff;
	case FLOOR:
		if (!fl.test(MapFlag::Real))
			be_trapped(&player.body.t_pos);
		goto move_stuff;
	default:
		turn.running = FALSE;
		if (is_monster(ch) || moat(nh.y, nh.x))
			fight(&nh, ch, player.weapon, FALSE);
		else {
			turn.running = FALSE;
			if (ch != STAIRS)
				turn.take = ch;
move_stuff:
			display().draw_tile(player.body.t_pos, level.at(player.body.t_pos));
			if (fl.test(MapFlag::Passage) && (level.at(player.old_pos) == DOOR
					|| level.flags_at(player.old_pos).test(MapFlag::Maze)))
				leave_room(&nh);
			if (fl.test(MapFlag::Maze) && !level.flags_at(player.old_pos).test(MapFlag::Maze))
				enter_room(&nh);
			bcopy(player.body.t_pos,nh);
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
	unsigned char ch;
	Creature *tp;

	if (!rp->r_flags.test(RoomFlag::Gone) && !game().player.body.t_flags.test(ISBLIND))
		for (j = rp->r_pos.y; j < rp->r_pos.y + rp->r_max.y; j++)
			for (k = rp->r_pos.x; k < rp->r_pos.x + rp->r_max.x; k++) {
				ch = winat(j, k);
				/* move(j, k); Why do this,?????? */
				if (is_monster(ch)) {
					tp = wake_monster(j, k);
					if (tp == NULL)
					{
						continue;
					}
					if (tp->t_oldch == ' ' && !rp->r_flags.test(RoomFlag::Dark)
						&& !game().player.body.t_flags.test(ISBLIND))
							tp->t_oldch = game().level.at(j, k);
				}
			}
}

/*
 * be_trapped:
 *	The guy stepped on a trap.... Make him pay.
 */
static
Trap
be_trapped(coord *tc)
{
	Trap tr;
	int index;
	rogue::Player &player = game().player;

	game().turn.count = game().turn.running = FALSE;
	index = INDEX(tc->y, tc->x);
	game().level.map[index] = TRAP;
	tr = game().level.flags[index].trap();
	player.was_trapped = TRUE;
	switch (tr) {
	case Trap::Door:
		descend("you fell into a trap!");
		break;
	case Trap::Bear:
		player.no_move += bear_time();
		msg("you are caught in a bear trap");
		break;
	case Trap::Sleep:
		player.no_command += sleep_time();
		player.body.t_flags.unset(ISRUN);
		msg("a {}mist envelops you and you fall asleep",
			noterse("strange white "));
		break;
	case Trap::Arrow:
		if (swing(player.body.t_stats.s_lvl-1, player.body.t_stats.s_arm, 1)) {
			player.body.t_stats.s_hpt -= roll(1, 6);
			if (player.body.t_stats.s_hpt <= 0) {
				msg("an arrow killed you");
				death('a');
			} else
				msg("oh no! An arrow shot you");
		}
		else {
			Item *arrow;

			if ((arrow = new_item()) != NULL) {
				arrow->o_type = ItemKind::Weapon;
				arrow->set_which(WeaponType::Arrow);
				init_weapon(arrow, WeaponType::Arrow);
				arrow->o_count = 1;
				bcopy(arrow->o_pos,player.body.t_pos);
				fall(arrow, FALSE);
			}
			msg("an arrow shoots past you");
		}
		break;
	case Trap::Teleport:
		teleport();
		display().draw_tile(*tc, TRAP); /* since the hero's leaving, look()
						won't put it on for us */
		/*
		 * TRUE + 1 tells look() that this was a teleport trap
		 */
		player.was_trapped++;
		break;
	case Trap::Dart:
		if (swing(player.body.t_stats.s_lvl+1, player.body.t_stats.s_arm, 1)) {
			player.body.t_stats.s_hpt -= roll(1, 4);
			if (player.body.t_stats.s_hpt <= 0) {
				msg("a poisoned dart killed you");
				death('d');
			}
			if (!player.wears(Ring::SustainStrength) && !save(SaveThrow::Poison))
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
	game().level.depth++;
	if (*mesg == 0)
		msg(" ");
	new_level();
	msg("");
	msg("{}", mesg);
	if (!save(SaveThrow::Luck)) {
		msg("you are damaged by the fall");
		if ((game().player.body.t_stats.s_hpt -= roll(1,8)) <= 0)
			death('f');
	}
}

/*
 * rndmove:
 *	Move in a random direction if the monster/person is confused
 */
void
rndmove(Creature *who, coord *newmv)
{
	int x, y;
	unsigned char ch;
	Item *obj;

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
			for (obj = game().level.objects.first(); obj != NULL; obj = game().level.objects.after(obj))
				if (y == obj->o_pos.y && x == obj->o_pos.x)
					break;
			if (obj != NULL && obj->which<Scroll>() == Scroll::ScareMonster)
				goto bad;
		}
	}
	return;

bad:
	bcopy((*newmv),who->t_pos);
	return;
}
