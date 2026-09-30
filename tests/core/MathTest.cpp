#include <gtest/gtest.h>

#include "core/Math.hpp"

using rogue::sign;

TEST(Math, SignOfNegativeZeroAndPositive)
{
	EXPECT_EQ(sign(-7), -1);
	EXPECT_EQ(sign(-1), -1);
	EXPECT_EQ(sign(0), 0);
	EXPECT_EQ(sign(1), 1);
	EXPECT_EQ(sign(42), 1);
	static_assert(sign(-3) == -1 && sign(0) == 0 && sign(3) == 1);
}
