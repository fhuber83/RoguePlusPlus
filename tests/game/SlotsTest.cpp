#include <gtest/gtest.h>

#include "game/Slots.hpp"

using rogue::Slots;

namespace {

struct Thing {
	int value = 7;
};

}  // namespace

TEST(Slots, TakeFillsTheFirstEmptySlot)
{
	Slots<Thing, 3> slots;
	Thing *a = slots.take();
	Thing *b = slots.take();
	ASSERT_NE(a, nullptr);
	EXPECT_EQ(a->value, 7);
	EXPECT_EQ(slots.slot_of(a), 0);
	EXPECT_EQ(slots.slot_of(b), 1);

	EXPECT_TRUE(slots.release(a));
	EXPECT_FALSE(slots.used(0));
	EXPECT_EQ(slots.at(0), nullptr);
	Thing *c = slots.take();
	EXPECT_EQ(slots.slot_of(c), 0);
	EXPECT_EQ(slots.at(0), c);
}

TEST(Slots, FullSlotsGiveNothing)
{
	Slots<Thing, 2> slots;
	ASSERT_NE(slots.take(), nullptr);
	ASSERT_NE(slots.take(), nullptr);
	EXPECT_EQ(slots.take(), nullptr);
}

TEST(Slots, ReleaseOnlyWhatIsInASlot)
{
	Slots<Thing, 2> slots;
	Thing outside;
	Thing *a = slots.take();
	EXPECT_FALSE(slots.release(&outside));
	EXPECT_FALSE(slots.release(nullptr));
	EXPECT_EQ(slots.slot_of(&outside), -1);
	EXPECT_EQ(slots.slot_of(nullptr), -1);
	EXPECT_TRUE(slots.release(a));
}

// A released thing starts over when its slot is taken again
TEST(Slots, TakenThingsAreNew)
{
	Slots<Thing, 1> slots;
	slots.take()->value = 3;
	slots.release(slots.at(0));
	EXPECT_EQ(slots.take()->value, 7);
}

TEST(Slots, TakeAtFillsAGivenSlot)
{
	Slots<Thing, 3> slots;
	Thing *b = slots.take_at(2);
	ASSERT_NE(b, nullptr);
	EXPECT_EQ(slots.at(2), b);
	EXPECT_EQ(slots.take_at(2), nullptr);
	EXPECT_EQ(slots.take_at(3), nullptr);
	EXPECT_EQ(slots.take_at(-1), nullptr);
	EXPECT_EQ(slots.slot_of(slots.take()), 0);
	EXPECT_EQ(slots.at(3), nullptr);
	EXPECT_EQ(slots.at(-1), nullptr);
}

// Moving the slots keeps what they own where it is
TEST(Slots, MoveKeepsAddresses)
{
	Slots<Thing, 2> slots;
	Thing *a = slots.take();
	Slots<Thing, 2> moved = std::move(slots);
	EXPECT_EQ(moved.at(0), a);
}
