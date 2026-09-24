#include <gtest/gtest.h>

#include <vector>

#include "entities/List.hpp"

using rogue::List;

namespace {

std::vector<int> values(const List<int> &list)
{
	std::vector<int> out;
	for (int *p : list)
		out.push_back(*p);
	return out;
}

}  // namespace

// push_front() adds to the front, like the old list_attach.
TEST(List, PushFront)
{
	int a = 1, b = 2, c = 3;
	List<int> list;
	EXPECT_TRUE(list.empty());
	EXPECT_EQ(list.first(), nullptr);
	list.push_front(&a);
	list.push_front(&b);
	list.push_front(&c);
	EXPECT_EQ(values(list), (std::vector<int>{3, 2, 1}));
	EXPECT_EQ(list.first(), &c);
	EXPECT_EQ(list.size(), 3u);
}

TEST(List, Neighbours)
{
	int a = 1, b = 2, c = 3, other = 4;
	List<int> list;
	list.push_front(&c);
	list.push_front(&b);
	list.push_front(&a);
	EXPECT_EQ(list.after(&a), &b);
	EXPECT_EQ(list.after(&c), nullptr);
	EXPECT_EQ(list.before(&b), &a);
	EXPECT_EQ(list.before(&a), nullptr);
	EXPECT_EQ(list.after(&other), nullptr);
	EXPECT_EQ(list.before(&other), nullptr);
}

// A walk stops when its body detaches the current entry, as the cleared
// l_next of a detached node used to stop it.
TEST(List, WalkStopsWhenCurrentIsDetached)
{
	int a = 1, b = 2, c = 3;
	List<int> list;
	list.push_front(&c);
	list.push_front(&b);
	list.push_front(&a);
	std::vector<int> seen;
	for (int *p = list.first(); p != nullptr; p = list.after(p)) {
		seen.push_back(*p);
		if (*p == 2)
			list.remove(p);
	}
	EXPECT_EQ(seen, (std::vector<int>{1, 2}));
	EXPECT_EQ(values(list), (std::vector<int>{1, 3}));
	EXPECT_FALSE(list.contains(&b));
}

// Detaching another entry during a walk keeps the walk going.
TEST(List, WalkContinuesWhenAnotherIsDetached)
{
	int a = 1, b = 2, c = 3;
	List<int> list;
	list.push_front(&c);
	list.push_front(&b);
	list.push_front(&a);
	std::vector<int> seen;
	for (int *p = list.first(); p != nullptr; p = list.after(p)) {
		seen.push_back(*p);
		if (*p == 1)
			list.remove(&b);
	}
	EXPECT_EQ(seen, (std::vector<int>{1, 3}));
}

TEST(List, InsertAfterAndBefore)
{
	int a = 1, b = 2, c = 3, d = 4;
	List<int> list;
	list.insert_after(nullptr, &b);		// empty: at the front
	list.insert_after(&b, &d);
	list.insert_before(&d, &c);
	list.insert_before(&b, &a);
	EXPECT_EQ(values(list), (std::vector<int>{1, 2, 3, 4}));
	list.remove(&a);
	list.remove(&a);	// not in the list: nothing happens
	EXPECT_EQ(values(list), (std::vector<int>{2, 3, 4}));
	list.clear();
	EXPECT_TRUE(list.empty());
}
