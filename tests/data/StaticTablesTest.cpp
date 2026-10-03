#include <gtest/gtest.h>

#include "core/Dice.hpp"
#include "rogue.h"

// Every monster attacks: a malformed table entry doesn't compile any more,
// but an empty one would mean the monster never swings.
TEST(StaticTables, EveryMonsterAttacks)
{
	for (int i = 0; i < 26; i++)
		EXPECT_FALSE(monsters[i].m_stats.s_dmg.empty()) << monsters[i].m_name;
}

// The flag column keeps the original bits, including the leprechaun whose
// ISGREED ended up in the carry column.
TEST(StaticTables, MonsterFlags)
{
	EXPECT_TRUE(monsters['B'-'A'].m_flags.test(CreatureFlag::Flying));
	EXPECT_EQ(monsters['G'-'A'].m_flags, CreatureFlag::Mean|CreatureFlag::Flying|CreatureFlag::Regen);
	EXPECT_EQ(monsters['L'-'A'].m_carry, 64);
	EXPECT_FALSE(monsters['L'-'A'].m_flags.any());
	EXPECT_EQ(monsters['O'-'A'].m_flags, CreatureFlags(CreatureFlag::Greedy));
	EXPECT_EQ(monsters['P'-'A'].m_flags.bits(), 0x0010);
}

// The help tables hold every line, and no empty one (the old tables ended
// with one, which help() no longer looks for).
TEST(StaticTables, HelpTables)
{
	for (const rogue::HelpLine &line : rogue::helpcoms)
		EXPECT_FALSE(line.h_desc.empty());
	for (const rogue::HelpLine &line : rogue::helpobjs)
		EXPECT_FALSE(line.h_desc.empty());
	EXPECT_EQ(rogue::helpcoms.front().h_desc, "F1     list of commands");
	EXPECT_EQ(rogue::helpcoms.back().h_desc, "D      list what has been discovered");
	EXPECT_EQ(rogue::helpobjs.front().glyphs(), std::string({static_cast<char>(rogue::FLOOR), ':', ' '}));
	EXPECT_EQ(rogue::helpobjs.back().glyphs(), "A-Z: ");
	EXPECT_EQ(rogue::helpobjs.back().h_desc, "26 different monsters");
}
