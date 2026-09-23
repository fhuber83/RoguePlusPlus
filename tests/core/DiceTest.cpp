#include <gtest/gtest.h>

#include "core/Dice.hpp"
#include "core/Random.hpp"

using rogue::Dice;
using rogue::parse_attacks;

TEST(Dice, ParsesSimpleExpression)
{
	EXPECT_EQ(Dice::parse("2d4"), (Dice{2, 4}));
	EXPECT_EQ(Dice::parse("0d0"), (Dice{0, 0}));
	EXPECT_EQ(Dice::parse("10d10"), (Dice{10, 10}));
}

TEST(Dice, RejectsMalformedExpressions)
{
	for (const char *text : {"", "d4", "2d", "2x4", "2d4x", "-1d4", "%%%d0", "2 d4"})
		EXPECT_FALSE(Dice::parse(text).has_value()) << text;
}

TEST(Dice, MinAndMax)
{
	constexpr Dice d{3, 8};
	static_assert(d.min() == 3 && d.max() == 24);
}

TEST(Dice, RollStaysWithinBounds)
{
	rogue::Random r{5};
	const Dice d{2, 6};
	for (int i = 0; i < 1000; i++) {
		const int v = d.roll(r);
		ASSERT_GE(v, d.min());
		ASSERT_LE(v, d.max());
	}
}

TEST(Dice, ParsesAttackLists)
{
	EXPECT_EQ(parse_attacks("1d8"), (std::vector<Dice>{{1, 8}}));
	EXPECT_EQ(parse_attacks("1d2/1d5/1d5"), (std::vector<Dice>{{1, 2}, {1, 5}, {1, 5}}));
}

TEST(Dice, MalformedAttackListIsEmpty)
{
	EXPECT_TRUE(parse_attacks("").empty());
	EXPECT_TRUE(parse_attacks("1d2/").empty());
	EXPECT_TRUE(parse_attacks("1d2/x").empty());
}
