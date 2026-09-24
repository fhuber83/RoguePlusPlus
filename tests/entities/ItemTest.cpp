#include <gtest/gtest.h>

#include "rogue.h"

// Every kind that can lie on the map shows as its glyph, and reads back.
TEST(ItemKind, GlyphRoundTrip)
{
	for (auto kind : {ItemKind::Potion, ItemKind::Scroll, ItemKind::Food,
			ItemKind::Weapon, ItemKind::Armor, ItemKind::Ring, ItemKind::Stick,
			ItemKind::Amulet, ItemKind::Gold})
		EXPECT_EQ(kind_of_glyph(glyph_of(kind)), kind) << static_cast<int>(kind);
}

// The glyphs are still the CP437 codes the map and help screen use.
TEST(ItemKind, Glyphs)
{
	EXPECT_EQ(glyph_of(ItemKind::Potion), POTION);
	EXPECT_EQ(glyph_of(ItemKind::Gold), GOLD);
	EXPECT_EQ(glyph_of(ItemKind::Missile), '*');
}

TEST(ItemKind, OtherGlyphsAreNoItems)
{
	EXPECT_EQ(kind_of_glyph(FLOOR), std::nullopt);
	EXPECT_EQ(kind_of_glyph(STAIRS), std::nullopt);
	EXPECT_EQ(kind_of_glyph('*'), std::nullopt);
	EXPECT_EQ(kind_of_glyph('A'), std::nullopt);
}

TEST(ItemFilter, Matches)
{
	ItemFilter food = ItemKind::Food;
	EXPECT_TRUE(food.is(ItemKind::Food));
	EXPECT_FALSE(food.is(ItemKind::Potion));
	EXPECT_FALSE(food.is_all());
	EXPECT_FALSE(food.is_callable());

	EXPECT_TRUE(ItemFilter::all().is_all());
	EXPECT_FALSE(ItemFilter::all().is(ItemKind::None));
	EXPECT_TRUE(ItemFilter::callable().is_callable());
	EXPECT_FALSE(ItemFilter::callable().is(ItemKind::None));
}
