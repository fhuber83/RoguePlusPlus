#include <gtest/gtest.h>

#include <string>

#include "core/Dice.hpp"
#include "core/Glyphs.hpp"
#include "entities/Creature.hpp"
#include "entities/MonsterCatalog.hpp"
#include "game/Help.hpp"

namespace rogue {

// Every monster attacks: a malformed table entry doesn't compile any more,
// but an empty one would mean the monster never swings.
TEST(StaticTables, EveryMonsterAttacks)
{
	for (int i = 0; i < 26; i++)
		EXPECT_FALSE(entities::monsters[i].stats.damage.empty()) << entities::monsters[i].name;
}

// The flag column keeps the original bits, including the leprechaun whose
// ISGREED ended up in the carry column.
TEST(StaticTables, MonsterFlags)
{
	EXPECT_TRUE(entities::monsters['B'-'A'].flags.test(CreatureFlag::Flying));
	EXPECT_EQ(entities::monsters['G'-'A'].flags, CreatureFlag::Mean|CreatureFlag::Flying|CreatureFlag::Regen);
	EXPECT_EQ(entities::monsters['L'-'A'].carry, 64);
	EXPECT_FALSE(entities::monsters['L'-'A'].flags.any());
	EXPECT_EQ(entities::monsters['O'-'A'].flags, CreatureFlags(CreatureFlag::Greedy));
	EXPECT_EQ(entities::monsters['P'-'A'].flags.bits(), 0x0010);
}

// The help tables hold every line, and no empty one (the old tables ended
// with one, which help() no longer looks for).
TEST(StaticTables, HelpTables)
{
	for (const HelpLine &line : helpcoms)
		EXPECT_FALSE(line.h_desc.empty());
	for (const HelpLine &line : helpobjs)
		EXPECT_FALSE(line.h_desc.empty());
	EXPECT_EQ(helpcoms.front().h_desc, "F1     list of commands");
	EXPECT_EQ(helpcoms.back().h_desc, "D      list what has been discovered");
	EXPECT_EQ(helpobjs.front().glyphs(), std::string({static_cast<char>(FLOOR), ':', ' '}));
	EXPECT_EQ(helpobjs.back().glyphs(), "A-Z: ");
	EXPECT_EQ(helpobjs.back().h_desc, "26 different monsters");
}

}  // namespace rogue
