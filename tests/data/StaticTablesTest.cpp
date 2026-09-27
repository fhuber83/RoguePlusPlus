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
	EXPECT_TRUE(monsters['B'-'A'].m_flags.test(ISFLY));
	EXPECT_EQ(monsters['G'-'A'].m_flags, ISMEAN|ISFLY|ISREGEN);
	EXPECT_EQ(monsters['L'-'A'].m_carry, 64);
	EXPECT_FALSE(monsters['L'-'A'].m_flags.any());
	EXPECT_EQ(monsters['O'-'A'].m_flags, CreatureFlags(ISGREED));
	EXPECT_EQ(monsters['P'-'A'].m_flags.bits(), 0x0010);
}
