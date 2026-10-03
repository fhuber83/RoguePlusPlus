#include "../support/ScriptedGame.hpp"

#include <gtest/gtest.h>

#include "entities/Item.hpp"
#include "entities/Stats.hpp"
#include "game/Game.hpp"
#include "game/Pool.hpp"
#include "items/Kinds.hpp"

namespace rogue {

namespace {

class PlayerTest : public test::ScriptedGame {
protected:
	// A ring of this kind and bonus, put on this hand
	static Item &put_on(Hand hand, Ring kind, short bonus)
	{
		Player &player = game().player;
		Item &ring = *new_item();
		ring.kind = ItemKind::Ring;
		ring.set_which(kind);
		ring.ac = bonus;
		ring.damage = ring.thrown_damage = "0d0";
		player.body.pack.push_front(ring);
		player.rings[hand] = game().pool.id_of(ring);
		return ring;
	}
};

}  // namespace

// Strength stays between 3 and 31
TEST(Stats, AddStrKeepsItsBounds)
{
	entities::str_t str = 16;
	entities::add_str(str, -14);
	EXPECT_EQ(str, 3u);
	entities::add_str(str, 40);
	EXPECT_EQ(str, 31u);
}

// The highest strength he has had is kept without what a ring adds
TEST_F(PlayerTest, ChangeStrengthRemembersTheMaximumWithoutRings)
{
	Player &player = game().player;
	player.body.stats.str = player.max_stats.str = 16;
	player.change_strength(2);
	EXPECT_EQ(player.body.stats.str, 18u);
	EXPECT_EQ(player.max_stats.str, 18u);

	put_on(Hand::Left, Ring::AddStrength, 3);
	player.change_strength(3);			// the ring's bonus
	EXPECT_EQ(player.body.stats.str, 21u);
	EXPECT_EQ(player.max_stats.str, 18u);	// 21 less the ring's 3
	player.change_strength(-20);
	EXPECT_EQ(player.body.stats.str, 3u);
	EXPECT_EQ(player.max_stats.str, 18u);
}

// The status line's armor class: his armor's, counting down from 11
TEST_F(PlayerTest, ArmorClassIsTheArmorsOrHisOwn)
{
	Player &player = game().player;
	ASSERT_TRUE(player.armor_item());		// a new game wears ring mail
	EXPECT_EQ(player.armor_class(), 11 - player.armor_item()->ac);
	player.armor.reset();
	player.body.stats.armor = 10;
	EXPECT_EQ(player.armor_class(), 1);
}

// Rings that always cost the same; no ring costs nothing
TEST_F(PlayerTest, RingFood)
{
	Player &player = game().player;
	EXPECT_EQ(player.ring_food(Hand::Left), 0);
	put_on(Hand::Left, Ring::Regeneration, 0);
	put_on(Hand::Right, Ring::Stealth, 0);
	EXPECT_EQ(player.ring_food(Hand::Left), 2);
	EXPECT_EQ(player.ring_food(Hand::Right), 1);
}

}  // namespace rogue
