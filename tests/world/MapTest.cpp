#include <gtest/gtest.h>

#include <set>

#include "rogue.h"

using rogue::world::INDEX;
using rogue::world::offmap;
using rogue::world::step_ok;

// The map covers rows 1 to maxrow - 1 of every column
TEST(Map, OffmapIsOutsideTheRowsAndColumns)
{
	EXPECT_TRUE(offmap(0, 10));
	EXPECT_TRUE(offmap(maxrow, 10));
	EXPECT_TRUE(offmap(5, -1));
	EXPECT_TRUE(offmap(5, MAXCOLS));
	EXPECT_FALSE(offmap(1, 0));
	EXPECT_FALSE(offmap(maxrow - 1, MAXCOLS - 1));
}

// Every square on the map has its own index in Level::map
TEST(Map, IndexIsOneToOneOverTheMap)
{
	std::set<int> seen;
	for (int y = 1; y < maxrow; y++)
		for (int x = 0; x < MAXCOLS; x++) {
			const int i = INDEX(y, x);
			ASSERT_GE(i, 0);
			ASSERT_LT(i, static_cast<int>(std::size(rogue::Level{}.map)));
			seen.insert(i);
		}
	EXPECT_EQ(seen.size(), static_cast<std::size_t>((maxrow - 1) * MAXCOLS));
}

TEST(Map, StepOkOnFloorsNotWallsOrMonsters)
{
	for (unsigned char ch : {rogue::FLOOR, rogue::PASSAGE, rogue::DOOR, rogue::STAIRS, rogue::TRAP, rogue::GOLD, rogue::POTION})
		EXPECT_TRUE(step_ok(ch)) << int(ch);
	for (unsigned char ch : {static_cast<unsigned char>(' '), rogue::VWALL, rogue::HWALL, rogue::ULWALL, rogue::URWALL, rogue::LLWALL, rogue::LRWALL})
		EXPECT_FALSE(step_ok(ch)) << int(ch);
	for (unsigned char ch = 'A'; ch <= 'Z'; ch++)
		EXPECT_FALSE(step_ok(ch)) << ch;
}

TEST(Map, TrapNames)
{
	EXPECT_EQ(rogue::world::tr_name(rogue::Trap::Door), "a trapdoor");
	EXPECT_EQ(rogue::world::tr_name(rogue::Trap::Arrow), "an arrow trap");
	EXPECT_EQ(rogue::world::tr_name(rogue::Trap::Sleep), "a sleeping gas trap");
	EXPECT_EQ(rogue::world::tr_name(rogue::Trap::Bear), "a beartrap");
	EXPECT_EQ(rogue::world::tr_name(rogue::Trap::Teleport), "a teleport trap");
	EXPECT_EQ(rogue::world::tr_name(rogue::Trap::Dart), "a poison dart trap");
}
