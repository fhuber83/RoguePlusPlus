#include <gtest/gtest.h>

#include <type_traits>

#include "world/MapFlags.hpp"

using rogue::MapFlag;
using rogue::MapFlags;
using rogue::Trap;

// One byte, as the level's grid and the save file hold it
static_assert(sizeof(MapFlags) == 1);
static_assert(std::is_trivially_default_constructible_v<MapFlags>);
static_assert(std::is_trivially_copyable_v<MapFlags>);

// The bits are the original F_* values, which old saves hold
TEST(MapFlags, OriginalBits)
{
	EXPECT_EQ(MapFlags(MapFlag::Passage).bits(), 0x40);
	EXPECT_EQ(MapFlags(MapFlag::Maze).bits(), 0x20);
	EXPECT_EQ(MapFlags(MapFlag::Real).bits(), 0x10);
	EXPECT_EQ(MapFlags(MapFlag::Maze | MapFlag::Real).bits(), 0x30);
}

TEST(MapFlags, SetTestUnset)
{
	MapFlags f = MapFlag::Real;
	EXPECT_TRUE(f.test(MapFlag::Real));
	EXPECT_FALSE(f.test(MapFlag::Passage));
	f.set(MapFlag::Passage);
	EXPECT_TRUE(f.test(MapFlag::Passage | MapFlag::Maze));
	f.unset(MapFlag::Real);
	EXPECT_EQ(f.bits(), 0x40);
}

// The passage number and the trap kind share the low bits with nothing else
TEST(MapFlags, PassageNumberKeepsTheFlags)
{
	MapFlags f = MapFlag::Passage | MapFlag::Real;
	EXPECT_EQ(f.passage(), 0);
	f.set_passage(12);
	EXPECT_EQ(f.passage(), 12);
	EXPECT_EQ(f.bits(), 0x5c);
	f.set_passage(3);
	EXPECT_EQ(f.passage(), 3);
	EXPECT_TRUE(f.test(MapFlag::Passage));
}

TEST(MapFlags, TrapKind)
{
	MapFlags f = MapFlags::from_bits(0);
	f.set_trap(Trap::Dart);
	EXPECT_EQ(f.trap(), Trap::Dart);
	EXPECT_EQ(f.bits(), 5);
	f.set_trap(Trap::Door);
	EXPECT_EQ(f.trap(), Trap::Door);
	EXPECT_FALSE(f.test(MapFlag::Real));
}
