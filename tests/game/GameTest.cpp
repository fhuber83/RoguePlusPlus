#include <gtest/gtest.h>

#include <optional>

#include "core/KindTable.hpp"
#include "core/Maybe.hpp"
#include "entities/Creature.hpp"
#include "entities/Item.hpp"
#include "game/Game.hpp"
#include "game/NewGame.hpp"
#include "game/Pool.hpp"
#include "items/ItemCatalog.hpp"
#include "items/KindInfo.hpp"
#include "items/Kinds.hpp"
#include "world/Room.hpp"

namespace rogue {

// A new game has the defaults the original globals were initialized with.
TEST(Options, Defaults)
{
	Options o;
	EXPECT_EQ(o.name, "Rodney");
	EXPECT_EQ(o.fruit, "Slime Mold");
	EXPECT_EQ(o.macro, "v");
	EXPECT_EQ(o.score_file, "rogue.scr");
	EXPECT_EQ(o.save_file, "rogue.sav");
	EXPECT_EQ(o.menu, "on");
	EXPECT_EQ(o.screen, "");
	EXPECT_FALSE(o.monochrome);
	EXPECT_FALSE(o.brief());
}

TEST(Options, BriefWhenTerseOrExpert)
{
	Options o;
	o.terse = true;
	EXPECT_TRUE(o.brief());
	o.terse = false;
	o.expert = true;
	EXPECT_TRUE(o.brief());
}

// Each game starts from the catalog odds and accumulates its own copy.
TEST(Items, OddsAreCopiedPerGame)
{
	Items items;
	for (Scroll s : kinds<Scroll>())
		EXPECT_EQ(items.s_magic[s].mi_prob, items::s_magic_base[s].mi_prob);
	for (int i = 0; i < items::NUMTHINGS; i++)
		EXPECT_EQ(items.things[i].mi_prob, items::things_base[i].mi_prob);

	game().items = {};
	init_things();
	EXPECT_EQ(game().items.things[items::NUMTHINGS-1].mi_prob, 100);
	EXPECT_EQ(items::things_base[items::NUMTHINGS-1].mi_prob, 5);
	game().items = {};
}

TEST(Level, PassagesAreGoneAndDark)
{
	Level level;
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
	game().pool = Pool();
	for (int i = 0; i < MAXITEMS - 1; i++)
		ASSERT_TRUE(new_item());
	Maybe<Creature> c = new_creature();
	ASSERT_TRUE(c);
	EXPECT_EQ(new_item(), std::nullopt);
	EXPECT_EQ(new_creature(), std::nullopt);

	EXPECT_TRUE(discard(*c));
	EXPECT_NE(new_item(), std::nullopt);
	EXPECT_EQ(game().pool.total, MAXITEMS);

	Creature outside{};
	EXPECT_FALSE(discard(outside));
	game().pool = Pool();
}

// A discarded item can't be the one get_item() gave last: its slot may be
// taken by the next item made.
TEST(Pool, DiscardForgetsTheLastItemPicked)
{
	game().pool = Pool();
	Item &kept = *new_item();
	Item &gone = *new_item();
	game().turn.last_item = game().pool.id_of(kept);
	discard(gone);
	EXPECT_TRUE(refers_to(game().pool.item(game().turn.last_item), kept));
	discard(kept);
	EXPECT_EQ(game().turn.last_item, std::nullopt);
	game().pool = Pool();
}

// A monster after a discarded item goes for the hero instead.
TEST(Pool, DiscardSendsMonstersAfterTheHero)
{
	game().pool = Pool();
	game().level = Level();
	Item &obj = *new_item();
	Creature &mp = *new_creature();
	game().level.monsters.push_front(mp);
	mp.t_dest = *game().pool.id_of(obj);
	discard(obj);
	EXPECT_EQ(mp.t_dest, Destination(Hero{}));
	game().level = Level();
	game().pool = Pool();
}

}  // namespace rogue
