#include "../support/ScriptedGame.hpp"

#include <gtest/gtest.h>

#include <algorithm>

#include "core/Coord.hpp"
#include "core/Glyphs.hpp"
#include "game/Game.hpp"
#include "game/Movement.hpp"
#include "world/Level.hpp"
#include "world/MapFlags.hpp"
#include "world/RoomRef.hpp"

namespace rogue {

namespace {

// A level of nothing but one passage, shaped like an L:
//	(y 5, x 10..12), then down (y 6..8, x 12)
class Movement : public test::ScriptedGame {
protected:
	void SetUp() override
	{
		ScriptedGame::SetUp();
		world::Level &level = game().level;
		level = world::Level();
		std::ranges::fill(level.map, ' ');
		for (Coord c : {Coord{10, 5}, Coord{11, 5}, Coord{12, 5}, Coord{12, 6}, Coord{12, 7}, Coord{12, 8}}) {
			level.at(c) = PASSAGE;
			level.flags_at(c).set(MapFlag::Passage | MapFlag::Real);
		}
		Player &player = game().player;
		player.body.t_pos = player.old_pos = {10, 5};
		player.body.t_room = RoomRef::passage(0);
	}

	static void run(char dir, int dy, int dx)
	{
		do_run(dir);
		game().turn.after = true;
		do_move(dy, dx);
	}
};

}  // namespace

// Running along a passage follows it around a corner
TEST_F(Movement, RunTurnsTheCornerOfAPassage)
{
	Player &player = game().player;
	Turn &turn = game().turn;

	run('l', 0, 1);
	EXPECT_EQ(player.body.t_pos, (Coord{11, 5}));
	run('l', 0, 1);
	EXPECT_EQ(player.body.t_pos, (Coord{12, 5}));
	run('l', 0, 1);		// a wall ahead: the passage goes on down
	EXPECT_EQ(player.body.t_pos, (Coord{12, 6}));
	EXPECT_EQ(turn.run_dir, 'j');
	EXPECT_TRUE(turn.running);
}

// At a dead end the run stops, and the move takes no turn
TEST_F(Movement, RunStopsAtADeadEnd)
{
	Player &player = game().player;
	Turn &turn = game().turn;

	player.body.t_pos = player.old_pos = {12, 8};
	run('j', 1, 0);
	EXPECT_EQ(player.body.t_pos, (Coord{12, 8}));
	EXPECT_FALSE(turn.running);
	EXPECT_FALSE(turn.after);
}

// A walk into a wall stops where it is
TEST_F(Movement, WalkIntoAWallStaysPut)
{
	game().turn.after = true;
	do_move(-1, 0);
	EXPECT_EQ(game().player.body.t_pos, (Coord{10, 5}));
	EXPECT_FALSE(game().turn.after);
}

// A random step goes to a square next to it that can be stepped on, or
// nowhere
TEST_F(Movement, RndmoveStaysOnThePassage)
{
	const Creature &body = game().player.body;
	for (Coord start : {Coord{10, 5}, Coord{12, 5}, Coord{12, 7}}) {
		game().player.body.t_pos = start;
		for (int i = 0; i < 200; i++) {
			const Coord to = rndmove(body);
			EXPECT_LE(distance_sq(to, start), 2);
			EXPECT_EQ(game().level.at(to), PASSAGE) << to.x << "," << to.y;
		}
	}
}

}  // namespace rogue
