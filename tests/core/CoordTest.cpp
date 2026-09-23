#include <gtest/gtest.h>

#include <type_traits>

#include "core/Coord.hpp"

using rogue::Coord;

static_assert(std::is_trivially_default_constructible_v<Coord>,
	"Coord lives inside union thing and must stay trivial");

TEST(Coord, EqualityComparesBothAxes)
{
	EXPECT_EQ((Coord{1, 2}), (Coord{1, 2}));
	EXPECT_NE((Coord{1, 2}), (Coord{2, 1}));
}

TEST(Coord, Arithmetic)
{
	constexpr Coord a{3, 4}, b{1, -2};
	static_assert(a + b == Coord{4, 2});
	static_assert(a - b == Coord{2, 6});
	Coord c = a;
	c += b;
	EXPECT_EQ(c, (Coord{4, 2}));
}

TEST(Coord, DistanceIsSquaredEuclidean)
{
	static_assert(rogue::distance_sq({0, 0}, {3, 4}) == 25);
	static_assert(rogue::distance_sq({5, 5}, {5, 5}) == 0);
}
