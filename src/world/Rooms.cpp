/*
 * Rooms at play time: entering and leaving one, which room a square is
 * in, and what the rogue can see from where it stands.
 *
 * rnd_pos(), enter_room() and leave_room() come from rooms.c; roomin(),
 * diag_ok() and cansee() from chase.c; door_open() from move.c.
 *
 * rooms.c	1.4 (A.I. Design)	12/16/84
 * chase.c	1.32	(A.I. Design) 12/12/84
 * move.c	1.4 (A.I. Design)	12/22/84
 */

#include "rogue.h"

namespace rogue::world {

namespace {

/*
 * door_open:
 *	Called to illuminate a room.  If it is dark, remove anything
 *	that might move.
 */
void
door_open(const Room &rp)
{
	int j, k;
	unsigned char ch;
	Maybe<Creature> tp;

	if (!rp.r_flags.test(RoomFlag::Gone) && !game().player.body.t_flags.test(ISBLIND))
		for (j = rp.r_pos.y; j < rp.r_pos.y + rp.r_max.y; j++)
			for (k = rp.r_pos.x; k < rp.r_pos.x + rp.r_max.x; k++) {
				ch = winat(j, k);
				/* move(j, k); Why do this,?????? */
				if (is_monster(ch)) {
					tp = wake_monster(j, k);
					if (!tp)
					{
						continue;
					}
					if (tp->t_oldch == ' ' && !rp.r_flags.test(RoomFlag::Dark)
						&& !game().player.body.t_flags.test(ISBLIND))
							tp->t_oldch = game().level.at(j, k);
				}
			}
}

}  // namespace

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
		const Room &r = level.rooms[i];
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
	if (distance_sq({x, y}, player.body.t_pos) < LAMPDIST)
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
rnd_pos(const Room &rp)
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
	const Room &rp = level.room(*in);
	door_open(rp);
	if (!rp.r_flags.test(RoomFlag::Dark) && !game().player.body.t_flags.test(ISBLIND) && !rp.r_flags.test(RoomFlag::Maze))
		for (y = rp.r_pos.y; y < rp.r_max.y + rp.r_pos.y; y++) {
			for (x = rp.r_pos.x; x < rp.r_max.x + rp.r_pos.x; x++) {
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
	unsigned char floor;
	unsigned char ch;
	rogue::Player &player = game().player;

	const Room &rp = game().level.room(*player.body.t_room);
	player.body.t_room = game().level.passage_at(cp);
	floor = (rp.r_flags.test(RoomFlag::Dark) && !player.body.t_flags.test(ISBLIND)) ? ' ' : FLOOR;
	if (rp.r_flags.test(RoomFlag::Maze))
		floor = PASSAGE;
	for (y = rp.r_pos.y + 1; y < rp.r_max.y + rp.r_pos.y - 1; y++)
		for (x = rp.r_pos.x + 1; x < rp.r_max.x + rp.r_pos.x - 1; x++)
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
	door_open(rp);
}

/*
 * teleport comes from wizard.c (wizard.c	1.4 (AI Design)	12/14/84).
 */

/*
 * teleport:
 *	Bamf the hero someplace else
 */
void
teleport()
{
	int rm;
	Coord c;
	rogue::Player &player = game().player;

	display().draw_tile(player.body.t_pos, game().level.at(player.body.t_pos));
	do
	{
		rm = rnd_room();
		c = rnd_pos(game().level.rooms[rm]);
	} while (!(step_ok(winat(c.y, c.x))));
	if (RoomRef::room(rm) != player.body.t_room)
	{
		leave_room(player.body.t_pos);
		player.body.t_pos = c;
		enter_room(player.body.t_pos);
	}
	else
	{
		player.body.t_pos = c;
		look(true);
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
	game().turn.running = false;
	flush_type();
	/*
	 * Teleportation can be a confusing experience
	 */
	if (player.body.t_flags.test(ISHUH))
		lengthen(Event::Unconfuse, rnd(4)+2);
	else
		fuse(Event::Unconfuse, rnd(4)+2);
	player.body.t_flags.set(ISHUH);
}

}  // namespace rogue::world
