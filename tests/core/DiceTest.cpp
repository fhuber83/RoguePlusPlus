#include <gtest/gtest.h>

#include <vector>

#include "core/Dice.hpp"
#include "core/Random.hpp"

using rogue::Dice;
using rogue::Attacks;

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

TEST(Attacks, ParsesAttackLists)
{
	auto one = Attacks::parse("1d8");
	ASSERT_TRUE(one);
	EXPECT_EQ(std::vector<Dice>(one->begin(), one->end()), (std::vector<Dice>{{1, 8}}));
	auto three = Attacks::parse("1d2/1d5/1d5");
	ASSERT_TRUE(three);
	EXPECT_EQ(std::vector<Dice>(three->begin(), three->end()), (std::vector<Dice>{{1, 2}, {1, 5}, {1, 5}}));
}

TEST(Attacks, RejectsMalformedLists)
{
	for (const char *text : {"", "1d2/", "1d2/x", "/1d2", "1d1/1d1/1d1/1d1/1d1"})
		EXPECT_FALSE(Attacks::parse(text).has_value()) << text;
}

TEST(Attacks, LiteralsConvertAtCompileTime)
{
	constexpr Attacks bite = "1d2/1d5/1d5";
	static_assert(bite.size() == 3 && *bite.begin() == Dice{1, 2});
	static_assert(Attacks("4d1") == Attacks(Dice{4, 1}));
}

TEST(Attacks, NoneIsNotZeroDice)
{
	constexpr Attacks none;
	constexpr Attacks zero = "0d0";
	static_assert(none.empty() && zero.size() == 1);
	EXPECT_NE(none, zero);
}

TEST(Attacks, TextRoundTrips)
{
	for (const char *text : {"0d0", "1d8", "1d2/1d5/1d5", "3d4/3d4/2d5/1d1", "10d10"})
		EXPECT_EQ(Attacks::parse(text)->to_string(), text);
	EXPECT_EQ(Attacks().to_string(), "");
}
