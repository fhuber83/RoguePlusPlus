#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <vector>

#include "rogue.h"

// A new game has the defaults the original globals were initialized with.
TEST(Options, Defaults)
{
	rogue::Options o;
	EXPECT_STREQ(o.name, "Rodney");
	EXPECT_STREQ(o.fruit, "Slime Mold");
	EXPECT_STREQ(o.macro, "v");
	EXPECT_STREQ(o.score_file, "rogue.scr");
	EXPECT_STREQ(o.save_file, "rogue.sav");
	EXPECT_STREQ(o.menu, "on");
	EXPECT_STREQ(o.screen, "");
	EXPECT_FALSE(o.monochrome);
	EXPECT_FALSE(o.brief());
}

TEST(Options, BriefWhenTerseOrExpert)
{
	rogue::Options o;
	o.terse = true;
	EXPECT_TRUE(o.brief());
	o.terse = false;
	o.expert = true;
	EXPECT_TRUE(o.brief());
}

// setenv_from_file() fills game().options from a rogue.opt file.
TEST(Options, ReadFromFile)
{
	auto path = std::filesystem::temp_directory_path() / "rogue_options_test.opt";
	FILE *f = std::fopen(path.c_str(), "w");
	ASSERT_NE(f, nullptr);
	std::fputs("# comment\nname = Optimus\nfruit=Kumquat\nmenu=sel\n"
		"scorefile=my.scr\nfruit_is_not_a_label=x\n", f);
	std::fclose(f);

	game().options = {};
	ASSERT_TRUE(setenv_from_file(path.c_str()));
	std::filesystem::remove(path);

	EXPECT_STREQ(game().options.name, "Optimus");
	EXPECT_STREQ(game().options.fruit, "Kumquat");
	EXPECT_STREQ(game().options.menu, "sel");
	EXPECT_STREQ(game().options.score_file, "my.scr");
	EXPECT_STREQ(game().options.macro, "v");
	game().options = {};
}

// Long values are cut to the size of their buffer.
TEST(Options, LongValuesAreTruncated)
{
	auto path = std::filesystem::temp_directory_path() / "rogue_options_long.opt";
	FILE *f = std::fopen(path.c_str(), "w");
	ASSERT_NE(f, nullptr);
	std::fputs("fruit=abcdefghijklmnopqrstuvwxyz\n", f);
	std::fclose(f);

	game().options = {};
	ASSERT_TRUE(setenv_from_file(path.c_str()));
	std::filesystem::remove(path);

	EXPECT_LT(std::strlen(game().options.fruit), sizeof game().options.fruit);
	game().options = {};
}

// Each game starts from the catalog odds and accumulates its own copy.
TEST(Items, OddsAreCopiedPerGame)
{
	rogue::Items items;
	for (int i = 0; i < MAXSCROLLS; i++)
		EXPECT_EQ(items.s_magic[i].mi_prob, s_magic_base[i].mi_prob);
	for (int i = 0; i < NUMTHINGS; i++)
		EXPECT_EQ(items.things[i].mi_prob, things_base[i].mi_prob);

	game().items = {};
	init_things();
	EXPECT_EQ(game().items.things[NUMTHINGS-1].mi_prob, 100);
	EXPECT_EQ(things_base[NUMTHINGS-1].mi_prob, 5);
	game().items = {};
}

TEST(Level, PassagesAreGoneAndDark)
{
	rogue::Level level;
	for (const auto &p : level.passages) {
		EXPECT_TRUE(p.r_flags.test(RoomFlag::Gone));
		EXPECT_TRUE(p.r_flags.test(RoomFlag::Dark));
		EXPECT_FALSE(p.r_flags.test(RoomFlag::Maze));
	}
	EXPECT_EQ(level.depth, 1);
}

// Creatures and items come from separate pools that share one count, like
// the single pool of the original.
TEST(Pool, CreaturesAndItemsShareTheLimit)
{
	game().pool = rogue::Pool();
	std::vector<Item *> items;
	for (int i = 0; i < MAXITEMS - 1; i++)
		items.push_back(new_item());
	Creature *c = new_creature();
	ASSERT_NE(c, nullptr);
	EXPECT_EQ(new_item(), nullptr);
	EXPECT_EQ(new_creature(), nullptr);

	EXPECT_EQ(discard(c), 1);
	EXPECT_NE(new_item(), nullptr);
	EXPECT_EQ(game().pool.total, MAXITEMS);

	Creature outside{};
	EXPECT_EQ(discard(&outside), 0);
	game().pool = rogue::Pool();
}
