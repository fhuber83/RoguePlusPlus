#include <gtest/gtest.h>

#include <cstddef>
#include <set>

#include "core/Glyphs.hpp"
#include "game/Game.hpp"
#include "world/Map.hpp"
#include "world/Trap.hpp"
#include "world/Traps.hpp"

namespace rogue {

using world::INDEX;
using world::offmap;
using world::step_ok;

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
			ASSERT_LT(i, static_cast<int>(std::size(Level{}.map)));
			seen.insert(i);
		}
	EXPECT_EQ(seen.size(), static_cast<std::size_t>((maxrow - 1) * MAXCOLS));
}

TEST(Map, StepOkOnFloorsNotWallsOrMonsters)
{
	for (unsigned char ch : {FLOOR, PASSAGE, DOOR, STAIRS, TRAP, GOLD, POTION})
		EXPECT_TRUE(step_ok(ch)) << int(ch);
	for (unsigned char ch : {static_cast<unsigned char>(' '), VWALL, HWALL, ULWALL, URWALL, LLWALL, LRWALL})
		EXPECT_FALSE(step_ok(ch)) << int(ch);
	for (unsigned char ch = 'A'; ch <= 'Z'; ch++)
		EXPECT_FALSE(step_ok(ch)) << ch;
}

TEST(Map, TrapNames)
{
	EXPECT_EQ(world::tr_name(Trap::Door), "a trapdoor");
	EXPECT_EQ(world::tr_name(Trap::Arrow), "an arrow trap");
	EXPECT_EQ(world::tr_name(Trap::Sleep), "a sleeping gas trap");
	EXPECT_EQ(world::tr_name(Trap::Bear), "a beartrap");
	EXPECT_EQ(world::tr_name(Trap::Teleport), "a teleport trap");
	EXPECT_EQ(world::tr_name(Trap::Dart), "a poison dart trap");
}

}  // namespace rogue
