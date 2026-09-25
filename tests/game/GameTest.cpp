#include <gtest/gtest.h>

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

// A discarded item can't be the one get_item() gave last: its address may be
// reused by the next item made.
TEST(Pool, DiscardForgetsTheLastItemPicked)
{
	game().pool = rogue::Pool();
	Item *kept = new_item();
	Item *gone = new_item();
	game().turn.last_item = kept;
	discard(gone);
	EXPECT_EQ(game().turn.last_item, kept);
	discard(kept);
	EXPECT_EQ(game().turn.last_item, nullptr);
	game().pool = rogue::Pool();
}

// A monster after a discarded item goes for the hero instead.
TEST(Pool, DiscardSendsMonstersAfterTheHero)
{
	game().pool = rogue::Pool();
	game().level = rogue::Level();
	Item *obj = new_item();
	Creature *mp = new_creature();
	game().level.monsters.push_front(mp);
	mp->t_dest = &obj->o_pos;
	discard(obj);
	EXPECT_EQ(mp->t_dest, &hero);
	game().level = rogue::Level();
	game().pool = rogue::Pool();
}
