#include <gtest/gtest.h>

#include <optional>

#include "core/Maybe.hpp"

using rogue::Maybe;

TEST(Maybe, EmptyByDefault)
{
	constexpr Maybe<int> none;
	static_assert(!none && !none.has_value());
	EXPECT_EQ(none, std::nullopt);
	EXPECT_EQ(Maybe<int>(std::nullopt), none);
	EXPECT_THROW(none.value(), std::bad_optional_access);
}

TEST(Maybe, RefersToTheThing)
{
	int n = 3;
	Maybe<int> m = n;
	ASSERT_TRUE(m);
	EXPECT_EQ(&*m, &n);
	*m = 4;
	EXPECT_EQ(n, 4);
	EXPECT_EQ(&m.value(), &n);
	m.reset();
	EXPECT_FALSE(m);
}

TEST(Maybe, ComparesByIdentity)
{
	int a = 1, b = 1;
	EXPECT_EQ(Maybe<int>(a), Maybe<int>(a));
	EXPECT_NE(Maybe<int>(a), Maybe<int>(b));
	EXPECT_NE(Maybe<int>(a), std::nullopt);
}

TEST(Maybe, FromPointer)
{
	int n = 0;
	int *none = nullptr;
	EXPECT_EQ(&*rogue::maybe(&n), &n);
	EXPECT_FALSE(rogue::maybe(none));
}

TEST(Maybe, ToConst)
{
	struct Thing {
		int x = 7;
	} t;
	Maybe<const Thing> c = Maybe<Thing>(t);
	EXPECT_EQ(c->x, 7);
	EXPECT_FALSE(Maybe<const Thing>(Maybe<Thing>()));
}

TEST(Maybe, RefersTo)
{
	int a = 1, b = 1;
	EXPECT_TRUE(rogue::refers_to(Maybe<int>(a), a));
	EXPECT_FALSE(rogue::refers_to(Maybe<int>(a), b));
	EXPECT_FALSE(rogue::refers_to(Maybe<int>(), a));
	EXPECT_TRUE(Maybe<const int>(a) == Maybe<int>(a));
}
