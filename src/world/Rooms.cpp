/*
 * Rooms at play time: entering and leaving one, which room a square is
 * in, and what the rogue can see from where it stands.
 *
 * rnd_pos(), enter_room() and leave_room() come from rooms.c; roomin(),
 * diag_ok() and cansee() from chase.c.
 *
 * rooms.c	1.4 (A.I. Design)	12/16/84
 * chase.c	1.32	(A.I. Design) 12/12/84
 */

#include "rogue.h"

namespace rogue::world {

/*
 * roomin:
 *	Find	what room some coordinates are in. NULL	means they aren't
 *	in any room.
 */
struct room *
roomin(coord *cp)
{
	struct room *rp;
	unsigned char *fp;

	for	(rp = game().level.rooms; rp	<= &game().level.rooms[MAXROOMS-1]; rp++)
		if (cp->x < rp->r_pos.x + rp->r_max.x && rp->r_pos.x <= cp->x
		 && cp->y < rp->r_pos.y + rp->r_max.y && rp->r_pos.y <= cp->y)
			return rp;
	fp = &flat(cp->y, cp->x);
	if (*fp & F_PASS)
		return	&game().level.passages[*fp &	F_PNUM];
#ifdef DEBUG
	debug("in some bizarre place ({}, {})", unc(*cp));
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

	if (game().player.body.t_flags.test(ISBLIND))
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
 * rnd_pos:
 *	Pick a random spot in a room
 */
void
rnd_pos(struct room *rp, coord *cp)
{
	cp->x = rp->r_pos.x + rnd(rp->r_max.x - 2) + 1;
	cp->y = rp->r_pos.y + rnd(rp->r_max.y - 2) + 1;
}

/*
 * enter_room:
 *	Code that is executed whenver you appear in a room
 */
void
enter_room(coord *cp)
{
	struct room *rp;
	int y, x;
	Creature *tp;

	rp = proom = roomin(cp);
	if (game().turn.bailout || (rp->r_flags.test(RoomFlag::Gone) && !rp->r_flags.test(RoomFlag::Maze))) {
#ifdef DEBUG
		msg("in a gone room");
#endif //DEBUG
		return;
	}
	door_open(rp);
	if (!rp->r_flags.test(RoomFlag::Dark) && !game().player.body.t_flags.test(ISBLIND) && !rp->r_flags.test(RoomFlag::Maze))
		for (y = rp->r_pos.y; y < rp->r_max.y + rp->r_pos.y; y++) {
			for (x = rp->r_pos.x; x < rp->r_max.x + rp->r_pos.x; x++) {
				/*
				 * Displaying monsters is all handled in the
				 * chase code now
				 */
				tp = moat(y, x);
				if (tp == NULL || !see_monst(tp))
					display().draw_tile({x, y}, chat(y, x));
				else {
					tp->t_oldch = chat(y,x);
					display().draw_tile({x, y}, tp->t_disguise);
				}
			}
		}
}

/*
 * leave_room:
 *	Code for when we exit a room
 */
void
leave_room(coord *cp)
{
	int y, x;
	struct room *rp;
	unsigned char floor;
	unsigned char ch;

	rp = proom;
	proom = &game().level.passages[flat(cp->y, cp->x) & F_PNUM];
	floor = (rp->r_flags.test(RoomFlag::Dark) && !game().player.body.t_flags.test(ISBLIND)) ? ' ' : FLOOR;
	if (rp->r_flags.test(RoomFlag::Maze))
		floor = PASSAGE;
	for (y = rp->r_pos.y + 1; y < rp->r_max.y + rp->r_pos.y - 1; y++)
		for (x = rp->r_pos.x + 1; x < rp->r_max.x + rp->r_pos.x - 1; x++)
			switch (ch = display().tile_at({x, y})) {
			case ' ':
			case PASSAGE:
			case TRAP:
			case STAIRS:
				break;
			case FLOOR:
				if (floor == ' ')
					display().draw_tile({x, y}, ' ');
				break;
			default:
				/*
				 * to check for monster, we have to strip out
				 * standout bit (the glyph has none)
				 */
				if (ismonster(ch))
				{
					if (game().player.body.t_flags.test(SEEMONST)) {
						display().draw_tile({x, y}, ch, TileStyle::Inverse);
						break;
					} else
						moat(y, x)->t_oldch = '@';
				}
				display().draw_tile({x, y}, floor);
				break;
			}
	door_open(rp);
}

}  // namespace rogue::world
