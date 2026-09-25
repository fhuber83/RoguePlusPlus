#pragma once

#include <array>
#include <cstddef>
#include <memory>

namespace rogue {

/*
 * A fixed number of numbered slots, each empty or owning one T on the heap.
 * The creatures and items in play live in these (see rogue::Pool): the slot
 * number is a thing's identity in a saved game, and a thing keeps its address
 * until it is released.
 *
 * take() fills the first empty slot, so things get the numbers they had in
 * the original's single array. A released thing is destroyed at once; a
 * pointer kept to it dangles, where the original's slot kept its contents
 * until it was reused.
 *
 * Member names avoid the lowercase macros of the legacy headers.
 */
template <typename T, std::size_t N>
class Slots {
public:
	static constexpr int capacity = static_cast<int>(N);

	// A new T in the first empty slot, or nullptr when every slot is taken
	T *take()
	{
		for (auto &slot : slots_)
			if (slot == nullptr)
				return (slot = std::make_unique<T>()).get();
		return nullptr;
	}

	// A new T in the given slot, or nullptr when it is taken or out of range
	T *take_at(int slot)
	{
		if (!in_range(slot) || slots_[slot] != nullptr)
			return nullptr;
		return (slots_[slot] = std::make_unique<T>()).get();
	}

	// Destroy the T in its slot; false if it isn't in one
	bool release(const T *thing)
	{
		int slot = slot_of(thing);
		if (slot < 0)
			return false;
		slots_[slot].reset();
		return true;
	}

	// The T in a slot, or nullptr when it is empty or out of range
	T *at(int slot) const { return in_range(slot) ? slots_[slot].get() : nullptr; }

	bool used(int slot) const { return at(slot) != nullptr; }

	// The slot holding thing, or -1 when none does
	int slot_of(const T *thing) const
	{
		if (thing == nullptr)
			return -1;
		for (int i = 0; i < capacity; i++)
			if (slots_[i].get() == thing)
				return i;
		return -1;
	}

private:
	static bool in_range(int slot) { return slot >= 0 && slot < capacity; }

	std::array<std::unique_ptr<T>, N> slots_;
};

}  // namespace rogue
