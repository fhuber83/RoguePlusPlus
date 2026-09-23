#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

#include "core/Flags.hpp"

namespace {
enum class Color : std::uint8_t { Red = 1, Green = 2, Blue = 4 };
}
template <>
inline constexpr bool rogue::enable_flags<Color> = true;

using Colors = rogue::Flags<Color>;

static_assert(std::is_trivially_default_constructible_v<Colors>);
static_assert(sizeof(Colors) == sizeof(Color));

TEST(Flags, SetTestClear)
{
	Colors c = Colors::none();
	EXPECT_FALSE(c.any());
	c.set(Color::Red);
	EXPECT_TRUE(c.test(Color::Red));
	EXPECT_FALSE(c.test(Color::Green));
	c.unset(Color::Red);
	EXPECT_FALSE(c.any());
}

TEST(Flags, CombineWithOr)
{
	constexpr Colors c = Color::Red | Color::Blue;
	static_assert(c.test(Color::Red) && c.test(Color::Blue) && !c.test(Color::Green));
	static_assert(c.bits() == 5);
}

TEST(Flags, TestMaskMatchesAnyBit)
{
	constexpr Colors c = Color::Green;
	static_assert(c.test(Color::Green | Color::Blue));
	static_assert(!c.test(Color::Red | Color::Blue));
}

TEST(Flags, SetWithValue)
{
	Colors c = Colors::none();
	c.set(Color::Green, true);
	EXPECT_TRUE(c.test(Color::Green));
	c.set(Color::Green, false);
	EXPECT_FALSE(c.test(Color::Green));
}

TEST(Flags, ZeroInitializedAsStatic)
{
	static Colors c;
	EXPECT_FALSE(c.any());
}
