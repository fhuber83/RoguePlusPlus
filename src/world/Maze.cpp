/*
 * Maze drawing routines.  Based on the algorithm presented in the
 * December 1981 Byte "How to Build a Maze" by David Matuszek.
 *
 * maze.c	1.4		(A.I. Design)	12/14/84
 */

#include "world/Maze.hpp"

#include <array>

#include "core/Config.hpp"
#include "core/Coord.hpp"
#include "core/Glyphs.hpp"
#include "game/Game.hpp"
#include "game/Messages.hpp"
#include "world/Map.hpp"
#include "world/MapFlags.hpp"
#include "world/Room.hpp"
#include "world/Rooms.hpp"

namespace rogue::world {

constexpr int MAXFRNT = 100;

constexpr unsigned char FRONTIER = 'F';
constexpr unsigned char NOTHING = ' ';

namespace {

int frcnt, ny, nx, topy, topx;
int maxx, maxy;
std::array<int, MAXFRNT> fr_y, fr_x;	/* the frontier */

void	new_frontier(int y, int x);
void	add_frnt(int y, int x);
void	con_frnt();
void	splat(int y, int x);
bool	maze_at(int y, int x);
bool	inrange(int y, int x);

}  // namespace

void
draw_maze(Room &rp)
{
	rogue::Level &level = game().level;

	maxx = maxy = 0;
	topy = rp.r_pos.y;
	if (topy == 0)
		topy = ++rp.r_pos.y;
	topx = rp.r_pos.x;
	/*
	 * Choose a random spot in the maze and initialize the frontier
	 * to be the immediate neighbors of this random spot.
	 */
	splat(topy, topx);
	new_frontier(topy, topx);
	/*
	 * While there are new frontiers, connect them to the path and
	 * possibly expand the frontier even more.
	 */
	while(frcnt)
	{
		con_frnt();
		new_frontier(ny, nx);
	}
	/*
	 * According to the Grand Beeking, every maze should have a loop
	 * Don't worry if you don't understand this.
	 */
	rp.r_max.x = maxx - rp.r_pos.x + 1;
	rp.r_max.y = maxy - rp.r_pos.y + 1;
	Coord spos;
	int psgcnt;
	do {
		static constexpr Coord ld[4] = {
			{-1,  0},
			{ 0,  1},
			{ 1,  0},
			{ 0, -1}
		};
		spos = rnd_pos(rp);
		psgcnt = 0;
		int sh = 1;
		for (Coord d : ld) {
			int y = d.y + spos.y, x = d.x + spos.x;
			if (!offmap(y, x) && level.at(y, x) == PASSAGE)
				psgcnt += sh;
			sh <<= 1;
		}
	} while (level.at(spos) == PASSAGE || psgcnt % 5);
	splat(spos.y, spos.x);
}

namespace {

void
new_frontier(int y, int x)
{
	add_frnt(y-2, x);
	add_frnt(y+2, x);
	add_frnt(y, x-2);
	add_frnt(y, x+2);
}

void
add_frnt(int y, int x)
{
	rogue::Level &level = game().level;

	if constexpr (rogue::config::debug_checks)
		if (frcnt == MAXFRNT - 1)
			debug("MAZE DRAWING ERROR #3");
	if (inrange(y, x) && level.at(y, x) == NOTHING)
	{
		level.at(y, x) = FRONTIER;
		fr_y[frcnt] = y;
		fr_x[frcnt++] = x;
	}
}

/*
 * Connect randomly to one of the adjacent points in the spanning tree
 */
void
con_frnt()
{
	/*
	 * Choose a random frontier
	 */
	int n = rnd(frcnt);
	ny = fr_y[n];
	nx = fr_x[n];
	fr_y[n] = fr_y[frcnt-1];
	fr_x[n] = fr_x[--frcnt];

	/*
	 * Count and collect the adjacent points we can connect to
	 */
	std::array<int, 4> choice;
	int cnt = 0;
	if (maze_at(ny-2, nx))
		choice[cnt++] = 0;
	if (maze_at(ny+2, nx))
		choice[cnt++] = 1;
	if (maze_at(ny, nx-2))
		choice[cnt++] = 2;
	if (maze_at(ny, nx+2))
		choice[cnt++] = 3;
	/*
	 * Choose one of the open places, connect to it and
	 * then the task is complete
	 */
	int which = choice[rnd(cnt)];
	splat(ny, nx);
	int ydelt = 0, xdelt = 0;
	switch(which)
	{
		case 0: which = 1; ydelt = -1; break;
		case 1: which = 0; ydelt = 1; break;
		case 2: which = 3; xdelt = -1; break;
		case 3: which = 2; xdelt = 1;
		break;
	}
	int y = ny + ydelt;
	int x = nx + xdelt;
	if (inrange(y, x))
		splat(y, x);
}

bool
maze_at(int y, int x)
{
	return (inrange(y, x) && game().level.at(y, x) == PASSAGE);
}

void
splat(int y, int x)
{
	rogue::Level &level = game().level;

	level.at(y, x) = PASSAGE;
	level.flags_at(y, x) = MapFlag::Maze | MapFlag::Real;
	if (x > maxx)
		maxx = x;
	if (y > maxy)
		maxy = y;
}

bool
inrange(int y, int x)
{
	return y >= topy && y < topy + (maxrow + 1) / 3 && x >= topx && x < topx + MAXCOLS / 3;
}

}  // namespace

}  // namespace rogue::world
