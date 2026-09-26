#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "ui/ScreenDisplay.hpp"
#include "rogue.h"

using rogue::pool_problems;

namespace {

// Each test starts and ends with an empty game
class PoolCheck : public ::testing::Test {
protected:
	void SetUp() override { reset(); }
	void TearDown() override { reset(); }

	static void reset()
	{
		game().pool = rogue::Pool();
		game().level = rogue::Level();
		game().player = rogue::Player();
		game().items = rogue::Items();
		game().turn = rogue::Turn();
	}

	static std::string problems()
	{
		std::string all;
		for (const std::string &p : pool_problems(game()))
			all += p + "\n";
		return all;
	}
};

}  // namespace

TEST_F(PoolCheck, EmptyGameIsFine)
{
	EXPECT_EQ(problems(), "");
}

TEST_F(PoolCheck, EachThingIsListedOnce)
{
	Item *obj = new_item();
	Creature *tp = new_creature();
	EXPECT_NE(problems(), "");		// neither is listed

	game().level.objects.push_front(obj);
	game().level.monsters.push_front(tp);
	EXPECT_EQ(problems(), "");

	game().player.body.t_pack.push_front(obj);
	EXPECT_NE(problems(), "");		// on the floor and in the pack
	game().level.objects.remove(obj);
	EXPECT_EQ(problems(), "");

	discard(obj);
	EXPECT_NE(problems(), "");		// free but still in the pack
}

TEST_F(PoolCheck, MonsterPacksCount)
{
	Creature *tp = new_creature();
	game().level.monsters.push_front(tp);
	Item *obj = new_item();
	tp->t_pack.push_front(obj);
	EXPECT_EQ(problems(), "");
}

TEST_F(PoolCheck, WhatAMonsterIsAfter)
{
	rogue::Level &level = game().level;
	Creature *tp = new_creature();
	level.monsters.push_front(tp);
	Item *obj = new_item();
	level.objects.push_front(obj);

	for (coord *dest : {static_cast<coord *>(nullptr), &game().player.body.t_pos, &level.rooms[3].r_gold,
			&level.passages[2].r_gold, &obj->o_pos}) {
		tp->t_dest = dest;
		EXPECT_EQ(problems(), "");
	}
	coord elsewhere{};
	tp->t_dest = &elsewhere;
	EXPECT_NE(problems(), "");

	// A carried item is not a destination
	level.objects.remove(obj);
	game().player.body.t_pack.push_front(obj);
	tp->t_dest = &obj->o_pos;
	EXPECT_NE(problems(), "");
}

TEST_F(PoolCheck, RoomsAreRoomsOrPassages)
{
	struct room elsewhere{};
	Creature *tp = new_creature();
	game().level.monsters.push_front(tp);
	tp->t_room = &game().level.passages[0];
	game().player.body.t_room = &game().level.rooms[0];
	game().player.old_room = &game().level.rooms[MAXROOMS - 1];
	EXPECT_EQ(problems(), "");
	tp->t_room = &elsewhere;
	EXPECT_NE(problems(), "");
}

TEST_F(PoolCheck, WornItemsAreInThePack)
{
	Item *obj = new_item();
	game().level.objects.push_front(obj);
	game().player.rings[1] = obj;
	EXPECT_NE(problems(), "");
	game().level.objects.remove(obj);
	game().player.body.t_pack.push_front(obj);
	EXPECT_EQ(problems(), "");
}

TEST_F(PoolCheck, TheLastItemPickedIsInUse)
{
	Item outside{};
	game().turn.last_item = &outside;
	EXPECT_NE(problems(), "");
	Item *obj = new_item();
	game().player.body.t_pack.push_front(obj);
	game().turn.last_item = obj;
	EXPECT_EQ(problems(), "");
}

TEST_F(PoolCheck, TheCountIsRight)
{
	game().pool.total = 1;
	EXPECT_NE(problems(), "");
}

// New games and the levels below them start out consistent.
TEST_F(PoolCheck, GeneratedLevels)
{
	auto &screen_display = dynamic_cast<rogue::ui::ScreenDisplay &>(display());
	screen_display.set_animations(false);	// new_level() wipes the screen
	int things = 0;
	for (rogue::Random::Seed seed : {1u, 5u, 42u, 4242u}) {
		reset();
		rogue::rng().reseed(seed);
		init_player();			// as main() sets up a game
		init_things();
		init_names();
		init_colors();
		init_stones();
		init_materials();
		for (int depth = 1; depth <= 26; depth++) {
			game().level.depth = depth;
			new_level();
			EXPECT_EQ(problems(), "") << "seed " << seed << " depth " << depth;
			things += game().pool.total;
		}
	}
	EXPECT_GT(things, 4 * 26 * 5);		// the levels do hold monsters and items
	screen_display.set_animations(true);
}
