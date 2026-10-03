#include <gtest/gtest.h>

#include <optional>
#include <string>

#include "core/Random.hpp"
#include "entities/Creature.hpp"
#include "entities/Item.hpp"
#include "game/Game.hpp"
#include "game/Id.hpp"
#include "game/NewGame.hpp"
#include "game/Pool.hpp"
#include "items/Kinds.hpp"
#include "ui/Display.hpp"
#include "ui/ScreenDisplay.hpp"
#include "world/LevelGenerator.hpp"
#include "world/Room.hpp"
#include "world/RoomRef.hpp"

namespace rogue {

namespace {

// Each test starts and ends with an empty game
class PoolCheck : public ::testing::Test {
protected:
	void SetUp() override { reset(); }
	void TearDown() override { reset(); }

	static void reset()
	{
		game().pool = Pool();
		game().level = Level();
		game().player = Player();
		game().items = Items();
		game().turn = Turn();
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
	Item &obj = *new_item();
	Creature &tp = *new_creature();
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
	Creature &tp = *new_creature();
	game().level.monsters.push_front(tp);
	Item &obj = *new_item();
	tp.t_pack.push_front(obj);
	EXPECT_EQ(problems(), "");
}

TEST_F(PoolCheck, WhatAMonsterIsAfter)
{
	Level &level = game().level;
	Creature &tp = *new_creature();
	level.monsters.push_front(tp);
	Item &obj = *new_item();
	level.objects.push_front(obj);

	for (std::optional<Destination> dest : {std::optional<Destination>(), std::optional<Destination>(Hero{}),
			std::optional<Destination>(Gold{RoomRef::room(3)}), std::optional<Destination>(Gold{RoomRef::passage(2)}),
			std::optional<Destination>(*game().pool.id_of(obj))}) {
		tp.t_dest = dest;
		EXPECT_EQ(problems(), "");
	}
	tp.t_dest = Gold{RoomRef::passage(world::MAXPASS)};
	EXPECT_NE(problems(), "");
	tp.t_dest = ItemId{MAXITEMS - 1};	// a free slot
	EXPECT_NE(problems(), "");

	// A carried item is not a destination
	level.objects.remove(obj);
	game().player.body.t_pack.push_front(obj);
	tp.t_dest = *game().pool.id_of(obj);
	EXPECT_NE(problems(), "");
}

TEST_F(PoolCheck, RoomsAreRoomsOrPassages)
{
	Creature &tp = *new_creature();
	game().level.monsters.push_front(tp);
	tp.t_room = RoomRef::passage(world::MAXPASS - 1);
	game().player.body.t_room = RoomRef::room(0);
	game().player.old_room = RoomRef::room(world::MAXROOMS - 1);
	EXPECT_EQ(problems(), "");
	tp.t_room = RoomRef::room(world::MAXROOMS);
	EXPECT_NE(problems(), "");
	tp.t_room = std::nullopt;
	game().player.old_room = RoomRef::passage(-1);
	EXPECT_NE(problems(), "");
}

TEST_F(PoolCheck, WornItemsAreInThePack)
{
	Item &obj = *new_item();
	game().level.objects.push_front(obj);
	game().player.rings[Hand::Right] = game().pool.id_of(obj);
	EXPECT_NE(problems(), "");
	game().level.objects.remove(obj);
	game().player.body.t_pack.push_front(obj);
	EXPECT_EQ(problems(), "");
}

TEST_F(PoolCheck, TheLastItemPickedIsInUse)
{
	game().turn.last_item = ItemId{MAXITEMS - 1};	// a free slot
	EXPECT_NE(problems(), "");
	Item &obj = *new_item();
	game().player.body.t_pack.push_front(obj);
	game().turn.last_item = game().pool.id_of(obj);
	EXPECT_EQ(problems(), "");
}

// Lists keep Ids: one whose thing was discarded without being taken out is
// reported, not followed
TEST_F(PoolCheck, DiscardedButListed)
{
	Item &obj = *new_item();
	game().level.objects.push_front(obj);
	Creature &tp = *new_creature();
	game().level.monsters.push_front(tp);
	EXPECT_EQ(problems(), "");
	discard(obj);
	discard(tp);
	EXPECT_NE(problems(), "");
	EXPECT_EQ(game().level.objects.first(), std::nullopt);
	EXPECT_EQ(game().level.monsters.first(), std::nullopt);
}

TEST_F(PoolCheck, TheCountIsRight)
{
	game().pool.total = 1;
	EXPECT_NE(problems(), "");
}

// New games and the levels below them start out consistent.
TEST_F(PoolCheck, GeneratedLevels)
{
	auto &screen_display = dynamic_cast<ui::ScreenDisplay &>(ui::display());
	screen_display.set_animations(false);	// new_level() wipes the screen
	int things = 0;
	for (Random::Seed seed : {1u, 5u, 42u, 4242u}) {
		reset();
		rng().reseed(seed);
		init_player();			// as main() sets up a game
		init_things();
		init_names();
		init_colors();
		init_stones();
		init_materials();
		for (int depth = 1; depth <= 26; depth++) {
			game().level.depth = depth;
			world::new_level();
			EXPECT_EQ(problems(), "") << "seed " << seed << " depth " << depth;
			things += game().pool.total;
		}
	}
	EXPECT_GT(things, 4 * 26 * 5);		// the levels do hold monsters and items
	screen_display.set_animations(true);
}

}  // namespace rogue
