#include <gtest/gtest.h>

#include "core/Dice.hpp"
#include "rogue.h"

// Every monster attack string must parse, or roll_em() silently skips it.
TEST(StaticTables, AllMonsterAttacksParse)
{
	for (int i = 0; i < 26; i++) {
		const auto &m = monsters[i];
		EXPECT_FALSE(rogue::parse_attacks(m.m_stats.s_dmg).empty())
			<< m.m_name << ": \"" << m.m_stats.s_dmg << '"';
	}
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
