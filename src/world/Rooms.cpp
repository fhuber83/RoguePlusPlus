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
 *	Find	what room some coordinates are in. nullopt	means they aren't
 *	in any room.
 */
std::optional<RoomRef>
roomin(Coord cp)
{
	rogue::Level &level = game().level;

	for	(int i = 0; i < MAXROOMS; i++) {
		const struct room &r = level.rooms[i];
		if (cp.x < r.r_pos.x + r.r_max.x && r.r_pos.x <= cp.x
		 && cp.y < r.r_pos.y + r.r_max.y && r.r_pos.y <= cp.y)
			return RoomRef::room(i);
	}
	if (level.flags_at(cp).test(MapFlag::Passage))
		return	level.passage_at(cp);
	if constexpr (rogue::config::debug_checks)
		debug("in some bizarre place ({}, {})", cp.y, cp.x);
	game().turn.bailout = true;
	return std::nullopt;
}

/*
 * diag_ok:
 *	Check to see	if the move is legal if	it is diagonal
 */
bool
diag_ok(Coord sp, Coord ep)
{
	rogue::Level &level = game().level;

	if (ep.x == sp.x || ep.y	== sp.y)
		return	true;
	return (step_ok(level.at(ep.y, sp.x))	&& step_ok(level.at(sp.y, ep.x)));
}

/*
 * cansee:
 *	Returns true	if the hero can	see a certain coordinate.
 */
bool
cansee(int y, int x)
{
	std::optional<RoomRef> rer;
	rogue::Player &player = game().player;

	if (player.body.t_flags.test(ISBLIND))
		return	false;
	if (DISTANCE(y, x, player.body.t_pos.y, player.body.t_pos.x) < LAMPDIST)
		return	true;
	/*
	 * We can only see if the hero in the same room as
	 * the coordinate and the room is lit or if	it is close.
	 */
	rer	= roomin({x, y});
	return (rer	== player.body.t_room && !game().level.room(*rer).r_flags.test(RoomFlag::Dark));
}

/*
 * rnd_pos:
 *	Pick a random spot in a room
 */
Coord
rnd_pos(const struct room &rp)
{
	Coord cp;

	cp.x = rp.r_pos.x + rnd(rp.r_max.x - 2) + 1;
	cp.y = rp.r_pos.y + rnd(rp.r_max.y - 2) + 1;
	return cp;
}

/*
 * enter_room:
 *	Code that is executed whenver you appear in a room
 */
void
enter_room(Coord cp)
{
	struct room *rp;
	int y, x;
	Maybe<Creature> tp;
	rogue::Level &level = game().level;

	const std::optional<RoomRef> in = game().player.body.t_room = roomin(cp);
	// roomin() sets bailout when it finds no room
	if (game().turn.bailout || (level.room(*in).r_flags.test(RoomFlag::Gone) && !level.room(*in).r_flags.test(RoomFlag::Maze))) {
		if constexpr (rogue::config::debug_checks)
			debug("in a gone room");
		return;
	}
	rp = &level.room(*in);
	door_open(*rp);
	if (!rp->r_flags.test(RoomFlag::Dark) && !game().player.body.t_flags.test(ISBLIND) && !rp->r_flags.test(RoomFlag::Maze))
		for (y = rp->r_pos.y; y < rp->r_max.y + rp->r_pos.y; y++) {
			for (x = rp->r_pos.x; x < rp->r_max.x + rp->r_pos.x; x++) {
				/*
				 * Displaying monsters is all handled in the
				 * chase code now
				 */
				tp = moat(y, x);
				if (!tp || !see_monst(*tp))
					display().draw_tile({x, y}, level.at(y, x));
				else {
					tp->t_oldch = level.at(y, x);
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
leave_room(Coord cp)
{
	int y, x;
	struct room *rp;
	unsigned char floor;
	unsigned char ch;
	rogue::Player &player = game().player;

	rp = &game().level.room(*player.body.t_room);
	player.body.t_room = game().level.passage_at(cp);
	floor = (rp->r_flags.test(RoomFlag::Dark) && !player.body.t_flags.test(ISBLIND)) ? ' ' : FLOOR;
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
				if (is_monster(ch))
				{
					if (player.body.t_flags.test(SEEMONST)) {
						display().draw_tile({x, y}, ch, TileStyle::Inverse);
						break;
					} else
						moat(y, x)->t_oldch = '@';
				}
				display().draw_tile({x, y}, floor);
				break;
			}
	door_open(*rp);
}

}  // namespace rogue::world
