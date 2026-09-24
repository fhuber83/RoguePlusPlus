/*
 * Code	for one	creature to chase another
 *
 * chase.c	1.32	(A.I. Design) 12/12/84
 */

#include "rogue.h"

#define	DRAGONSHOT  5	/* one chance in DRAGONSHOT that a dragon will flame */

coord ch_ret;			/* Where chasing takes	you */

/*
 * runners:
 *	Make all the running monsters move.
 */
void
runners()
{
	THING *tp;
	int dist;

	for	(tp = game().level.monsters; tp	!= NULL; tp = next(tp)) {
		if (!on(*tp, ISHELD) && on(*tp, ISRUN)) {
			dist = DISTANCE(hero.y, hero.x, tp->t_pos.y, tp->t_pos.x);
			if	(!(on(*tp, ISSLOW) || (tp->t_type == 'S' && dist > 3)) || tp->t_turn)
				do_chase(tp);
			if (on(*tp, ISHASTE))
				do_chase(tp);
			dist = DISTANCE(hero.y, hero.x, tp->t_pos.y, tp->t_pos.x);
			if (on(*tp, ISFLY) && dist > 3)
				do_chase(tp);
			tp->t_turn ^= TRUE;
		}
	}
}

/*
 * do_chase:
 *	Make one thing chase another.
 */
void
do_chase(THING *th)
{
	int	mindist	= 32767, i, dist;
	bool door;
	THING *obj;
	struct room	*oroom;
	struct room	*rer, *ree;	/* room of chaser, room of chasee */
	coord target;				/* Temporary	destination for	chaser */

	rer	= th->t_room;		/* Find room of chaser */
	if (on(*th,	ISGREED) && rer->r_goldval == 0)
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
			&&	!on(*th, ISCANC) && rnd(DRAGONSHOT) == 0)
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
		for (obj = game().level.objects; obj != NULL; obj =	next(obj))
			if	(th->t_dest == &obj->o_pos) {
				byte oldchar;

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
				&& !on(game().player.body, SEEMONST))
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
	else if (on(game().player.body,	SEEMONST))
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
see_monst(THING *mp)
{
	rogue::Player &player = game().player;
	if (on(player.body, ISBLIND))
		return	FALSE;
	if (on(*mp,	ISINVIS) && !on(player.body,	CANSEE))
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
	  && ((player.weapon->o_flags & DIDFLASH) == 0))
	{
		player.weapon->o_flags |=	DIDFLASH;
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
	THING *tp;

	/*
	 * If we couldn't find him,	something is funny
	 */
	tp = moat(runner->y, runner->x);
	if (tp != NULL) {
		/*
		 *	Start the beastie running
		 */
		tp->t_flags |= ISRUN;
		tp->t_flags &= ~ISHELD;
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
 *	chasee(ee).	Returns	TRUE if	we want	to keep	on chasing later
 *	FALSE if we reach the goal.
 *
 *	@@ Wrong documentation: function is actually a void, there is no return
 */
void
chase(THING *tp, coord *ee)
{
	int	x, y;
	int	dist, thisdist;
	THING *obj;
	coord *er;
	byte ch;
	int	plcnt =	1;

	er = &tp->t_pos;
	/*
	 * If the thing is confused, let it	move randomly. Phantoms
	 * are slightly confused all of the	time, and bats are
	 * quite confused all the time
	 */
	if ((on(*tp, ISHUH)	&& rnd(5) != 0)	|| (tp->t_type == 'P' && rnd(5)	== 0)
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
			tp->t_flags &= ~ISHUH;
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
						for (obj = game().level.objects; obj != NULL; obj	= next(obj))
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
 * roomin:
 *	Find	what room some coordinates are in. NULL	means they aren't
 *	in any room.
 */
struct room *
roomin(coord *cp)
{
	struct room *rp;
	byte *fp;

	for	(rp = game().level.rooms; rp	<= &game().level.rooms[MAXROOMS-1]; rp++)
		if (cp->x < rp->r_pos.x + rp->r_max.x && rp->r_pos.x <= cp->x
		 && cp->y < rp->r_pos.y + rp->r_max.y && rp->r_pos.y <= cp->y)
			return rp;
	fp = &flat(cp->y, cp->x);
	if (*fp & F_PASS)
		return	&game().level.passages[*fp &	F_PNUM];
#ifdef DEBUG
	debug("in some bizarre place (%d, %d)", unc(*cp));
#endif //DEBUG
	game().turn.bailout = TRUE;
	return NULL;
}

/*
 * diag_ok:
 *	Check to see	if the move is legal if	it is diagonal
 */
bool
diag_ok(coord *sp, coord *ep)
{
	if (ep->x == sp->x || ep->y	== sp->y)
		return	TRUE;
	return (step_ok(chat(ep->y,	sp->x))	&& step_ok(chat(sp->y, ep->x)));
}

/*
 * cansee:
 *	Returns true	if the hero can	see a certain coordinate.
 */
bool
cansee(int y, int x)
{
	struct room *rer;
	coord tp;

	if (on(game().player.body, ISBLIND))
		return	FALSE;
	if (DISTANCE(y, x, hero.y, hero.x) < LAMPDIST)
		return	TRUE;
	/*
	 * We can only see if the hero in the same room as
	 * the coordinate and the room is lit or if	it is close.
	 */
	tp.y = y;
	tp.x = x;
	rer	= roomin(&tp);
	return (rer	== proom && !rer->r_flags.test(RoomFlag::Dark));
}

/*
 * find_dest:
 *	find	the proper destination for the monster
 */
coord *
find_dest(THING *tp)
{
	THING *obj;
	int prob;
	struct room *rp;

	if ((prob =	monsters[tp->t_type - 'A'].m_carry) <= 0 || tp->t_room == proom
	|| see_monst(tp))
		return &hero;
	rp = tp->t_room;
	for	(obj = game().level.objects;	obj != NULL; obj = next(obj))
	{
	if (obj->o_type == SCROLL && obj->o_which == S_SCARE)
		continue;
	if (roomin(&obj->o_pos) == rp && rnd(100) < prob)
	{
		for (tp = game().level.monsters; tp != NULL; tp = next(tp))
		if (tp->t_dest == &obj->o_pos)
			break;
		if	(tp == NULL)
		return &obj->o_pos;
	}
	}
	return &hero;
}
