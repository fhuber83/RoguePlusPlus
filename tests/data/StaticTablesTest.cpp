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
