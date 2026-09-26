/*
 * Code	for one	creature to chase another
 *
 * chase.c	1.32	(A.I. Design) 12/12/84
 */

#include "rogue.h"

namespace rogue::entities {

static void	do_chase(Creature *th);
static void	chase(Creature *tp, coord *ee);

#define	DRAGONSHOT  5	/* one chance in DRAGONSHOT that a dragon will flame */

static coord ch_ret;			/* Where chasing takes	you */

/*
 * runners:
 *	Make all the running monsters move.
 */
void
runners()
{
	Creature *tp;
	int dist;

	for (tp = game().level.monsters.first(); tp != NULL; tp = game().level.monsters.after(tp)) {
		if (!tp->t_flags.test(ISHELD) && tp->t_flags.test(ISRUN)) {
			dist = DISTANCE(hero.y, hero.x, tp->t_pos.y, tp->t_pos.x);
			if	(!(tp->t_flags.test(ISSLOW) || (tp->t_type == 'S' && dist > 3)) || tp->t_turn)
				do_chase(tp);
			/*
			 * do_chase() can end in attack(), which removes tp from the
			 * level (a Leprechaun or Nymph vanishes once it steals). Once
			 * that happens tp is a freed pool slot and must not be read
			 * again this turn; the loop still stops walking the list at
			 * this point, as in the original (see MODERNIZATION.md 6.4).
			 */
			if (!game().level.monsters.contains(tp))
				continue;
			if (tp->t_flags.test(ISHASTE))
				do_chase(tp);
			if (!game().level.monsters.contains(tp))
				continue;
			dist = DISTANCE(hero.y, hero.x, tp->t_pos.y, tp->t_pos.x);
			if (tp->t_flags.test(ISFLY) && dist > 3)
				do_chase(tp);
			if (!game().level.monsters.contains(tp))
				continue;
			tp->t_turn ^= TRUE;
		}
	}
}

/*
 * do_chase:
 *	Make one thing chase another.
 */
static void
do_chase(Creature *th)
{
	int	mindist	= 32767, i, dist;
	bool door;
	Item *obj;
	struct room	*oroom;
	struct room	*rer, *ree;	/* room of chaser, room of chasee */
	coord target;				/* Temporary	destination for	chaser */

	rer	= th->t_room;		/* Find room of chaser */
	if (th->t_flags.test(ISGREED) && rer->r_goldval == 0)
		th->t_dest = &hero;	/*	If gold	has been taken,	run after hero */
	ree	= proom;
	if (th->t_dest != &hero)	/*	Find room of chasee */
		ree = roomin(th->t_dest);
	if (ree == NULL)
		return;
	/*
	 * We don't	count doors as inside rooms for	this routine
	 */
	door = (chat(th->t_pos.y, th->t_pos.x) == DOOR);


	/*
	 * If the object of	our desire is in a different room,
	 * and we are not in a maze, run to	the door nearest to
	 * our goal.
	 */
over:
	if (rer != ree && !rer->r_flags.test(RoomFlag::Maze))
	{
		for (i	= 0; i < rer->r_nexits;	i++) {	/*	loop through doors */
			dist = DISTANCE(th->t_dest->y, th->t_dest->x,rer->r_exit[i].y, rer->r_exit[i].x);
			if	(dist <	mindist) {
				target = rer->r_exit[i];
				mindist = dist;
			}
		}
		if (door) {
			rer = &game().level.passages[flat(th->t_pos.y, th->t_pos.x) & F_PNUM];
			door = FALSE;
			goto over;
		}
	} else {
		target =	*th->t_dest;
		/*
		 * For	monsters which can fire	bolts at the poor hero,	we check to
		 * see	if (a) the hero	in on a	straight line from it, and (b) that
		 * it is within shooting distance, but	outside	of striking range.
		 */
		if ((th->t_type == 'D' || th->t_type == 'I')
			&&	(th->t_pos.y ==	hero.y || th->t_pos.x == hero.x
			 || abs(th->t_pos.y - hero.y) == abs(th->t_pos.x - hero.x))
			&&	((dist=DISTANCE(th->t_pos.y, th->t_pos.x, hero.y, hero.x)) > 2
			 && dist <= BOLT_LENGTH	* BOLT_LENGTH)
			&&	!th->t_flags.test(ISCANC) && rnd(DRAGONSHOT) == 0)
		{
			game().turn.running = FALSE;
			game().turn.delta.y = sign(hero.y - th->t_pos.y);
			game().turn.delta.x = sign(hero.x - th->t_pos.x);
			fire_bolt(&th->t_pos,&game().turn.delta,th->t_type == 'D' ? "flame" : "frost");
			return;
		}
	}
	/*
	 * This now	contains what we want to run to	this time
	 * so we run to it.	 If we hit it we either	want to	fight it
	 * or stop running
	 */
	chase(th, &target);
	if (ch_ret == hero) {
		attack(th);
		return;
	} else if (ch_ret == *th->t_dest) {
		for (obj = game().level.objects.first(); obj != NULL; obj = game().level.objects.after(obj))
			if	(th->t_dest == &obj->o_pos) {
				unsigned char oldchar;

				detach(game().level.objects, obj);
				attach(th->t_pack, obj);
				oldchar = chat(obj->o_pos.y, obj->o_pos.x) =
				th->t_room->r_flags.test(RoomFlag::Gone) ? PASSAGE : FLOOR;
				if (cansee(obj->o_pos.y, obj->o_pos.x))
					display().draw_tile(obj->o_pos, oldchar);
				th->t_dest = find_dest(th);
				break;
			}
	}
	if (th->t_type == 'F')
		return;
	/*
	 * If the chasing thing moved, update the screen
	 */
	if (th->t_oldch != '@') {
		if	(th->t_oldch ==	' ' && cansee(th->t_pos.y, th->t_pos.x)
			   && game().level.map[INDEX(th->t_pos.y,th->t_pos.x)] == FLOOR)
			display().draw_tile(th->t_pos, FLOOR);
		else if (th->t_oldch == FLOOR && !cansee(th->t_pos.y, th->t_pos.x)
				&& !game().player.body.t_flags.test(SEEMONST))
			display().draw_tile(th->t_pos, ' ');
		else
			display().draw_tile(th->t_pos, th->t_oldch);
	}
	oroom = th->t_room;
	if (!(ch_ret == th->t_pos))
	{
		if ((th->t_room = roomin(&ch_ret)) == NULL) {
			th->t_room	= oroom;
			return;
		}
		if (oroom != th->t_room)
			th->t_dest	= find_dest(th);
		th->t_pos = ch_ret;
	}

	if (see_monst(th)) {
		th->t_oldch = display().tile_at(ch_ret);
		display().draw_tile(ch_ret, th->t_disguise,
				(flat(ch_ret.y,ch_ret.x) & F_PASS) ? TileStyle::Inverse : TileStyle::Normal);
	}
	else if (game().player.body.t_flags.test(SEEMONST))
	{
		th->t_oldch = display().tile_at(ch_ret);
		display().draw_tile(ch_ret, th->t_type, TileStyle::Inverse);
	}
	else
		th->t_oldch = '@';

	if (th->t_oldch == FLOOR && oroom->r_flags.test(RoomFlag::Dark))
		th->t_oldch = ' ';
}

/*
 * see_monst:
 *	Return TRUE if the hero can see the monster
 */
bool
see_monst(Creature *mp)
{
	rogue::Player &player = game().player;
	if (player.body.t_flags.test(ISBLIND))
		return	FALSE;
	if (mp->t_flags.test(ISINVIS) && !player.body.t_flags.test(CANSEE))
		return	FALSE;
	if (DISTANCE(mp->t_pos.y, mp->t_pos.x, hero.y, hero.x) >= LAMPDIST &&
	  ((mp->t_room != proom || mp->t_room->r_flags.test(RoomFlag::Dark) ||
	  mp->t_room->r_flags.test(RoomFlag::Maze))))
		return FALSE;
	/*
	 * If we are seeing	the enemy of a vorpally	enchanted weapon for the first
	 * time, give the player a hint as to what that weapon is good for.
	 */
	if (player.weapon != NULL && mp->t_type == player.weapon->o_enemy
	  && !player.weapon->o_flags.test(DIDFLASH))
	{
		player.weapon->o_flags.set(DIDFLASH);
		msg(flashmsg, w_names[player.weapon->o_which], game().options.brief() ? "" : intense);
	}
	return TRUE;
}

/*
 * start_run:
 *	Set a monster running after something or stop it from running
 *	(for	when it	dies)
 */
void
start_run(coord *runner)
{
	Creature *tp;

	/*
	 * If we couldn't find him,	something is funny
	 */
	tp = moat(runner->y, runner->x);
	if (tp != NULL) {
		/*
		 *	Start the beastie running
		 */
		tp->t_flags.set(ISRUN);
		tp->t_flags.unset(ISHELD);
		tp->t_dest	= find_dest(tp);
	}
#ifdef DEBUG
	else
		debug("start_run: moat == NULL ???");
#endif //DEBUG
}

/*
 * chase:
 *	Find	the spot for the chaser(er) to move closer to the
 *	chasee(ee).
 */
static void
chase(Creature *tp, coord *ee)
{
	int	x, y;
	int	dist, thisdist;
	Item *obj;
	coord *er;
	unsigned char ch;
	int	plcnt =	1;

	er = &tp->t_pos;
	/*
	 * If the thing is confused, let it	move randomly. Phantoms
	 * are slightly confused all of the	time, and bats are
	 * quite confused all the time
	 */
	if ((tp->t_flags.test(ISHUH)	&& rnd(5) != 0)	|| (tp->t_type == 'P' && rnd(5)	== 0)
		|| (tp->t_type	== 'B' && rnd(2) == 0))
	{
		/*
		 * get	a valid	random move
		 */
		rndmove(tp,&ch_ret);
		dist =	DISTANCE(ch_ret.y, ch_ret.x, ee->y, ee->x);
		/*
		 * Small chance that it will become un-confused
		 */
		if (rnd(30) ==	17)
			tp->t_flags.unset(ISHUH);
	}
	/*
	 * Otherwise, find the empty spot next to the chaser that is
	 * closest to the chasee.
	 */
	else
	{
		int ey, ex;
		/*
		 * This will eventually hold where we move to get closer
		 * If we can't	find an	empty spot, we stay where we are.
		 */
		dist =	DISTANCE(er->y,	er->x, ee->y, ee->x);
		ch_ret	= *er;

		ey = er->y + 1;
		ex = er->x + 1;
		for (x	= er->x	- 1; x <= ex; x++)
		{
			for (y = er->y - 1; y <= ey; y++)
			{
				coord	tryp;

				tryp.x = x;
				tryp.y = y;
				if (offmap(y,	x) || !diag_ok(er, &tryp))
					continue;
				ch = winat(y,	x);
				if (step_ok(ch))
				{
					/*
					 * If it is a scroll, it might be	a scare	monster	scroll
					 * so we need to look it up to see what type it is.
					 */
					if (ch ==	SCROLL)
					{
						for (obj = game().level.objects.first(); obj != NULL; obj = game().level.objects.after(obj))
						{
							if (y ==	obj->o_pos.y &&	x == obj->o_pos.x)
								break;
						}
						if (obj != NULL && obj->o_which == S_SCARE)
							continue;
					}
					/*
					 * If we didn't find any scrolls at this place or	it
					 * wasn't	a scare	scroll,	then this place	counts
					 */
					thisdist = DISTANCE(y, x,	ee->y, ee->x);
					if (thisdist < dist)
					{
						plcnt = 1;
						ch_ret = tryp;
						dist	= thisdist;
					}
					else if (thisdist	== dist	&& rnd(++plcnt)	== 0)
					{
						ch_ret = tryp;
						dist	= thisdist;
					}
				}
			}
		}
	}
}

/*
 * find_dest:
 *	find	the proper destination for the monster
 */
coord *
find_dest(Creature *tp)
{
	Item *obj;
	int prob;
	struct room *rp;

	if ((prob =	monsters[tp->t_type - 'A'].m_carry) <= 0 || tp->t_room == proom
	|| see_monst(tp))
		return &hero;
	rp = tp->t_room;
	for (obj = game().level.objects.first(); obj != NULL; obj = game().level.objects.after(obj))
	{
	if (obj->o_type == ItemKind::Scroll && obj->o_which == S_SCARE)
		continue;
	if (roomin(&obj->o_pos) == rp && rnd(100) < prob)
	{
		for (tp = game().level.monsters.first(); tp != NULL; tp = game().level.monsters.after(tp))
		if (tp->t_dest == &obj->o_pos)
			break;
		if	(tp == NULL)
		return &obj->o_pos;
	}
	}
	return &hero;
}

/*
 * Code for handling the various special properties of the slime
 *
 * slime.c	1.0		(A.I. Design 1.42)	1/17/85
 */

/*
 * Slime_split:
 *	Called when it has been decided that A slime should divide itself
 */

static coord slimy;

static bool	new_slime(Creature *tp);

void
slime_split(Creature *tp)
{
	Creature *nslime;

	if (!new_slime(tp) || (nslime = new_creature()) == NULL)
		return;
	msg("The slime divides.  Ick!");
	new_monster(nslime, 'S', &slimy);
	if (cansee(slimy.y, slimy.x)) {
		nslime->t_oldch = chat(slimy.y, slimy.x);
		display().draw_tile(slimy, 'S');
	}
	start_run(&slimy);
}

static
bool
new_slime(Creature *tp)
{
	int y, x, ty, tx;
	bool ret;
	Creature *ntp;
	coord sp;

	ret = FALSE;
	tp->t_flags.set(ISFLY);
	if (!plop_monster((ty = tp->t_pos.y), (tx = tp->t_pos.x), &sp)) {
		/*
		 * There were no open spaces next to this slime, look for other
		 * slimes that might have open spaces next to them.
		 */
		for (y = ty -1; y <= ty+1; y++)
			for (x = tx-1; x <= tx+1; x++)
				if (winat(y, x) == 'S' && (ntp = moat(y, x))) {
					if (ntp->t_flags.test(ISFLY))
						continue;				/* Already done this one */
					if (new_slime(ntp)) {
						y = ty+2;
						x = tx +2;
					}
				}
	} else {
		ret = TRUE;
		slimy = sp;
	}
	tp->t_flags.unset(ISFLY);
	return ret;
}

/*
 * Pick an appropriate spot around a central spot for a new monster to spawn
 * (r, c): row, col of central spot
 * cp: pointer to coordinate for the new monster, if any
 * Return FALSE if no suitable spot around (r, c) is found
 *
 * Original return value was somewhat an abuse of the bool convention,
 * used both as TRUE/FALSE and as an integer for calculating odds.
 * To avoid that, 'inv_odds' was created for the rnd() call,
 * and 'appear' is now "strictly" boolean
 */
bool
plop_monster(int r, int c, coord *cp)
{
	int y, x, inv_odds = 0;
	bool appear = FALSE;
	unsigned char ch;

	for (y = r-1; y <= r+1; y++)
		for (x = c-1; x <= c+1; x++) {
			/*
			 * Don't put a monster in top of the player.
			 */
			if ((y == hero.y && x == hero.x) || offmap(y,x))
				continue;
			/*
			 * Or anything else nasty
			 */
			if (step_ok(ch = winat(y, x))) {
				if (ch == SCROLL && find_obj(y, x)->o_which == S_SCARE)
					continue;
				/*
				 * Get first available spot with 100% chance,
				 * then randomly change to next available spot, if any,
				 * with decreasing 1-to-n odds (50%, 33%, 25%, 20%,...)
				 */
				appear = TRUE;
				if (rnd(++inv_odds) == 0) {
					cp->y = y;
					cp->x = x;
				}
			}
		}
	return appear;
}

}  // namespace rogue::entities
