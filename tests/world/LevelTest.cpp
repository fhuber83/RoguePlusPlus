#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <set>

#include "core/Glyphs.hpp"
#include "core/Maybe.hpp"
#include "entities/Item.hpp"
#include "game/Game.hpp"	// IWYU pragma: keep (the lists find things through game().pool)
#include "game/Pool.hpp"
#include "world/Level.hpp"
#include "world/MapFlags.hpp"
#include "world/RoomRef.hpp"
#include "world/Trap.hpp"
#include "world/Traps.hpp"

namespace rogue {

// The map covers rows 1 to maxrow - 1 of every column
TEST(Level, OffmapIsOutsideTheRowsAndColumns)
{
	EXPECT_TRUE(world::Level::off_map({10, 0}));
	EXPECT_TRUE(world::Level::off_map({10, maxrow}));
	EXPECT_TRUE(world::Level::off_map({-1, 5}));
	EXPECT_TRUE(world::Level::off_map({MAXCOLS, 5}));
	EXPECT_FALSE(world::Level::off_map({0, 1}));
	EXPECT_FALSE(world::Level::off_map({MAXCOLS - 1, maxrow - 1}));
}

// Every square on the map has its own index in world::Level::map
TEST(Level, IndexIsOneToOneOverTheMap)
{
	std::set<int> seen;
	for (int y = 1; y < maxrow; y++)
		for (int x = 0; x < MAXCOLS; x++) {
			const int i = world::Level::index({x, y});
			ASSERT_GE(i, 0);
			ASSERT_LT(i, static_cast<int>(std::size(world::Level{}.map)));
			seen.insert(i);
		}
	EXPECT_EQ(seen.size(), static_cast<std::size_t>((maxrow - 1) * MAXCOLS));
}

TEST(Level, StepOkOnFloorsNotWallsOrMonsters)
{
	for (unsigned char ch : {FLOOR, PASSAGE, DOOR, STAIRS, TRAP, GOLD, POTION})
		EXPECT_TRUE(step_ok(ch)) << int(ch);
	for (unsigned char ch : {static_cast<unsigned char>(' '), VWALL, HWALL, ULWALL, URWALL, LLWALL, LRWALL})
		EXPECT_FALSE(step_ok(ch)) << int(ch);
	for (unsigned char ch = 'A'; ch <= 'Z'; ch++)
		EXPECT_FALSE(step_ok(ch)) << ch;
}

// What stands and lies on a square, and what the rogue sees there
TEST(Level, MonsterAndObjectAt)
{
	world::Level level;
	Item &gold = *new_item();	// lists name things by their pool slot
	gold.kind = ItemKind::Gold;
	gold.pos = {10, 5};
	Creature &mimic = *new_creature();
	mimic.type = 'X';
	mimic.disguise = STAIRS;
	mimic.pos = {11, 5};
	level.at({10, 5}) = GOLD;
	level.at({11, 5}) = FLOOR;
	level.objects.push_front(gold);
	level.monsters.push_front(mimic);

	EXPECT_TRUE(refers_to(level.object_at({10, 5}), gold));
	EXPECT_FALSE(level.object_at({11, 5}));
	EXPECT_TRUE(refers_to(level.monster_at({11, 5}), mimic));
	EXPECT_FALSE(level.monster_at({10, 5}));
	EXPECT_EQ(level.seen_at({10, 5}), GOLD);
	EXPECT_EQ(level.seen_at({11, 5}), STAIRS);	// the disguise
	level.objects.remove(gold);
	level.monsters.remove(mimic);
	discard(gold);
	discard(mimic);
}

// A square of a room is in it, a passage square in its passage, and a
// diagonal step needs both squares beside it open
TEST(Level, RoomAtAndDiagonals)
{
	world::Level level;
	level.rooms[4].pos = {30, 8};
	level.rooms[4].size = {10, 6};
	EXPECT_EQ(level.room_at({30, 8}), RoomRef::room(4));
	EXPECT_EQ(level.room_at({39, 13}), RoomRef::room(4));
	EXPECT_EQ(level.room_at({40, 13}), std::nullopt);

	level.flags_at({50, 3}).set(MapFlag::Passage);
	level.flags_at({50, 3}).set_passage(7);
	EXPECT_EQ(level.room_at({50, 3}), RoomRef::passage(7));

	level.at({31, 9}) = FLOOR;
	level.at({32, 10}) = FLOOR;
	level.at({32, 9}) = FLOOR;
	level.at({31, 10}) = VWALL;
	EXPECT_FALSE(level.diagonal_ok({31, 9}, {32, 10}));	// a wall at one corner
	level.at({31, 10}) = FLOOR;
	EXPECT_TRUE(level.diagonal_ok({31, 9}, {32, 10}));
	EXPECT_TRUE(level.diagonal_ok({31, 10}, {31, 9}));	// not diagonal
}

TEST(Level, TrapNames)
{
	EXPECT_EQ(world::tr_name(Trap::Door), "a trapdoor");
	EXPECT_EQ(world::tr_name(Trap::Arrow), "an arrow trap");
	EXPECT_EQ(world::tr_name(Trap::Sleep), "a sleeping gas trap");
	EXPECT_EQ(world::tr_name(Trap::Bear), "a beartrap");
	EXPECT_EQ(world::tr_name(Trap::Teleport), "a teleport trap");
	EXPECT_EQ(world::tr_name(Trap::Dart), "a poison dart trap");
}

}  // namespace rogue
