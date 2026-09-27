#include <gtest/gtest.h>

#include <string_view>
#include <type_traits>

#include "core/KindTable.hpp"

namespace {
enum class Fruit { Apple, Pear, Plum };
}
template <>
inline constexpr std::size_t rogue::kind_count<Fruit> = 3;

using rogue::KindTable;

// A drop-in for the C array it replaces: same size, copied with memcpy
static_assert(sizeof(KindTable<Fruit, int>) == sizeof(int[3]));
static_assert(std::is_trivially_copyable_v<KindTable<Fruit, int>>);
static_assert(KindTable<Fruit, int>::size() == 3);
static_assert(KindTable<Fruit, int, 4>::size() == 4);

constexpr KindTable<Fruit, std::string_view> names = {"apple", "pear", "plum"};
static_assert(names[Fruit::Pear] == "pear");

// A list of the wrong length doesn't compile, since the constructor is consteval:
//   constexpr KindTable<Fruit, int> short_list = {1, 2};

TEST(KindTable, IndexedByKind)
{
	KindTable<Fruit, int> count = {};
	count[Fruit::Plum] = 5;
	EXPECT_EQ(count[Fruit::Apple], 0);
	EXPECT_EQ(count[Fruit::Plum], 5);
	EXPECT_EQ(count.data()[2], 5);
}

TEST(KindTable, ValueInitializedIsZero)
{
	KindTable<Fruit, const char *> table = {};
	for (const char *p : table)
		EXPECT_EQ(p, nullptr);
}

TEST(KindTable, KindsInOrder)
{
	constexpr auto all = rogue::kinds<Fruit>();
	static_assert(all.size() == 3);
	EXPECT_EQ(all[0], Fruit::Apple);
	EXPECT_EQ(all[2], Fruit::Plum);
}
