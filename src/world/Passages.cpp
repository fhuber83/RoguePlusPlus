/*
 * Draw the connecting passages
 *
 * passages.c	1.4 (A.I. Design)	12/14/84
 */

#include "world/Passages.hpp"

#include <array>
#include <cstdlib>

#include "core/Config.hpp"
#include "core/Coord.hpp"
#include "core/Glyphs.hpp"
#include "core/Maybe.hpp"
#include "game/Game.hpp"
#include "game/Messages.hpp"
#include "world/Level.hpp"
#include "world/MapFlags.hpp"
#include "world/Room.hpp"

namespace rogue::world {

namespace {

void	conn(int r1, int r2);
void	door(Room &rm, Coord cp);
// Numbering the passages: the number of the one being numbered, and
// whether the next exit starts a new one
struct Numbering {
	int pnum = 0;
	bool newpnum = false;
};

void	passnum();
void	numpass(int y, int x, Numbering &num);
void	psplat(int y, int x);

/*
 * conn:
 *	Draw a corridor from a room in a certain direction.
 */
void
conn(int r1, int r2)
{
	Maybe<Room> rpt;
	int rm;
	int distance = 0, turn_distance;
	int direc;
	Coord del, turn_delta, spos, epos;
	world::Level &level = game().level;

	if (r1 < r2) {
		rm = r1;
		if (r1 + 1 == r2)
			direc = 'r';
		else
			direc = 'd';
	} else {
		rm = r2;
		if (r2 + 1 == r1)
			direc = 'r';
		else
			direc = 'd';
	}
	Room &rpf = level.rooms[rm];
	/*
	 * Set up the movement variables, in two cases:
	 * first drawing one down.
	 */
	if (direc == 'd') {
		int rmt = rm + 3;			/* room # of dest */
		rpt = level.rooms[rmt];			/* the destination room */
		del.x = 0;				/* direction of move */
		del.y = 1;
		/*
		 * If we are drawing from/to regular or maze rooms, we have
		 * to pick the spot we draw from/to
		 */
		if (!rpf.r_flags.test(RoomFlag::Gone) || rpf.r_flags.test(RoomFlag::Maze)) {
			spos.y = rpf.r_pos.y + rpf.r_max.y - 1;
			do {
				spos.x = rpf.r_pos.x + rnd(rpf.r_max.x - 2) + 1;
			} while (level.at(spos) == ' ');
		} else {
			spos.x = rpf.r_pos.x;
			spos.y = rpf.r_pos.y;
		}
		epos.y = rpt->r_pos.y;
		if (!rpt->r_flags.test(RoomFlag::Gone) || rpt->r_flags.test(RoomFlag::Maze)) {
			do {
				epos.x = rpt->r_pos.x + rnd(rpt->r_max.x - 2) + 1;
			} while (level.at(epos) == ' ');
		} else
			epos.x = rpt->r_pos.x;
		distance = abs(spos.y - epos.y) - 1;	/* distance to move */
		turn_delta.y = 0;			/* direction to turn */
		turn_delta.x = (spos.x < epos.x ? 1 : -1);
		turn_distance = abs(spos.x - epos.x);	/* how far to turn */
	} else if (direc == 'r') {			/* setup for moving right */
		int rmt = rm + 1;
		rpt = level.rooms[rmt];
		del.x = 1;
		del.y = 0;
		if (!rpf.r_flags.test(RoomFlag::Gone) || rpf.r_flags.test(RoomFlag::Maze)) {
			spos.x = rpf.r_pos.x + rpf.r_max.x-1;
			do {
				spos.y = rpf.r_pos.y + rnd(rpf.r_max.y-2)+1;
			} while (level.at(spos) == ' ');
		} else {
			spos.x = rpf.r_pos.x;
			spos.y = rpf.r_pos.y;
		}
		epos.x = rpt->r_pos.x;
		if (!rpt->r_flags.test(RoomFlag::Gone) || rpt->r_flags.test(RoomFlag::Maze)) {
			do {
				epos.y = rpt->r_pos.y + rnd(rpt->r_max.y-2)+1;
			} while (level.at(epos) == ' ');
		} else
			epos.y = rpt->r_pos.y;
		distance = abs(spos.x - epos.x) - 1;
		turn_delta.y = (spos.y < epos.y ? 1 : -1);
		turn_delta.x = 0;
		turn_distance = abs(spos.y - epos.y);
	}
	else if constexpr (rogue::config::debug_checks)
		debug("error in connection tables");
	int turn_spot = rnd(distance-1) + 1;
	/*
	 * Draw in the doors on either side of the passage or just put #'s
	 * if the rooms are gone.
	 */
	if (!rpf.r_flags.test(RoomFlag::Gone))
		door(rpf, spos);
	else
		psplat(spos.y, spos.x);
	if (rpt && !rpt->r_flags.test(RoomFlag::Gone))
		door(*rpt, epos);
	else
		psplat(epos.y, epos.x);
	/*
	 * Get ready to move...
	 */
	Coord curr = spos;
	while (distance)
	{
	/*
	 * Move to new position
	 */
	curr.x += del.x;
	curr.y += del.y;
	/*
	 * Check if we are at the turn place, if so do the turn
	 */
	if (distance == turn_spot)
	{
		while (turn_distance--)
		{
		psplat(curr.y, curr.x);
		curr.x += turn_delta.x;
		curr.y += turn_delta.y;
		}
	}
	/*
	 * Continue digging along
	 */
	psplat(curr.y, curr.x);
	distance--;
	}
	curr.x += del.x;
	curr.y += del.y;
	if (!(curr == epos)) {
	epos.x -= del.x;
	epos.y -= del.y;
	psplat(epos.y, epos.x);
	}
}

}  // namespace

/*
 * do_passages:
 *	Draw all the passages on a level.
 */
void
do_passages()
{
	/*
	 * Which rooms are next to each other, and so can be connected
	 */
	static constexpr std::array<std::array<bool, MAXROOMS>, MAXROOMS> next_to = {{
		{ 0, 1, 0, 1, 0, 0, 0, 0, 0 },
		{ 1, 0, 1, 0, 1, 0, 0, 0, 0 },
		{ 0, 1, 0, 0, 0, 1, 0, 0, 0 },
		{ 1, 0, 0, 0, 1, 0, 1, 0, 0 },
		{ 0, 1, 0, 1, 0, 1, 0, 1, 0 },
		{ 0, 0, 1, 0, 1, 0, 0, 0, 1 },
		{ 0, 0, 0, 1, 0, 0, 0, 1, 0 },
		{ 0, 0, 0, 0, 1, 0, 1, 0, 1 },
		{ 0, 0, 0, 0, 0, 1, 0, 1, 0 },
	}};
	struct Graph {
		std::array<bool, MAXROOMS> isconn{};	/* connection been made to room i? */
		bool ingraph = false;			/* this room in graph already? */
	};
	std::array<Graph, MAXROOMS> rdes{};

	/*
	 * starting with one room, connect it to a random adjacent room and
	 * then pick a new room to start with.
	 */
	int roomcount = 1;
	int r1 = rnd(MAXROOMS);
	int r2 = 0;
	rdes[r1].ingraph = true;
	do
	{
		/*
		 * find a room to connect with
		 */
		int j = 0;
		for (int i = 0; i < MAXROOMS; i++)
			if (next_to[r1][i] && !rdes[i].ingraph && rnd(++j) == 0)
				r2 = i;
		/*
		 * if no adjacent rooms are outside the graph, pick a new room
		 * to look from
		 */
		if (j == 0)
		{
			do
				r1 = rnd(MAXROOMS);
			while (!rdes[r1].ingraph);
		}
		/*
		 * otherwise, connect new room to the graph, and draw a tunnel
		 * to it
		 */
		else
		{
			rdes[r2].ingraph = true;
			conn(r1, r2);
			rdes[r1].isconn[r2] = true;
			rdes[r2].isconn[r1] = true;
			roomcount++;
		}
	} while (roomcount < MAXROOMS);

	/*
	 * attempt to add passages to the graph a random number of times so
	 * that there isn't always just one unique passage through it.
	 */
	for (int extra = rnd(5); extra > 0; extra--)
	{
		r1 = rnd(MAXROOMS);	/* a random room to look from */
		/*
		 * find an adjacent room not already connected
		 */
		int j = 0;
		for (int i = 0; i < MAXROOMS; i++)
			if (next_to[r1][i] && !rdes[r1].isconn[i] && rnd(++j) == 0)
				r2 = i;
		/*
		 * if there is one, connect it and look for the next added
		 * passage
		 */
		if (j != 0)
		{
			conn(r1, r2);
			rdes[r1].isconn[r2] = true;
			rdes[r2].isconn[r1] = true;
		}
	}
	passnum();
}

namespace {

/*
 * door:
 *	Add a door or possibly a secret door.  Also enters the door in
 *	the exits array of the room.
 */
void
door(Room &rm, Coord cp)
{
	int index = Level::index(cp);
	if (rnd(10) + 1 < game().level.depth && rnd(5) == 0)
	{
		game().level.map[index] = (cp.y == rm.r_pos.y || cp.y == rm.r_pos.y + rm.r_max.y - 1) ? HWALL : VWALL;
		game().level.flags[index].unset(MapFlag::Real);
	}
	else
		game().level.map[index] = DOOR;
	int xit = rm.r_nexits++;
	rm.r_exit[xit] = cp;
}

/*
 * passnum:
 *	Assign a number to each passageway
 */
void
passnum()
{
	Numbering num;
	for (Room &rp : game().level.passages)
		rp.r_nexits = 0;
	for (const Room &rp : game().level.rooms)
		for (int i = 0; i < rp.r_nexits; i++)
		{
			num.newpnum = true;	/* was a count (newpnum++), only ever tested */
			numpass(rp.r_exit[i].y, rp.r_exit[i].x, num);
		}
}
/*
 * numpass:
 *	Number a passageway square and its brethren
 */
void
numpass(int y, int x, Numbering &num)
{
	world::Level &level = game().level;

	if (Level::off_map({x, y}))
		return;
	MapFlags &fp = level.flags_at(y, x);
	if (fp.passage())
		return;
	if (num.newpnum) {
		num.pnum++;
		num.newpnum = false;
	}
	/*
	 * check to see if it is a door or secret door, i.e., a new exit,
	 * or a numerable type of place
	 */
	unsigned char ch = level.at(y, x);
	if (ch == DOOR || (!fp.test(MapFlag::Real) && ch != FLOOR)) {
		Room &rp = level.passages[num.pnum];
		rp.r_exit[rp.r_nexits].y = y;
		rp.r_exit[rp.r_nexits++].x = x;
	} else if (!fp.test(MapFlag::Passage))
		return;
	fp.set_passage(num.pnum);
	/*
	 * recurse on the surrounding places
	 */
	numpass(y + 1, x, num);
	numpass(y - 1, x, num);
	numpass(y, x + 1, num);
	numpass(y, x - 1, num);
}

void
psplat(int y, int x)
{
	int idx = Level::index({x, y});
	game().level.map[idx] = PASSAGE;
	game().level.flags[idx].set(MapFlag::Passage);
}

}  // namespace

}  // namespace rogue::world
