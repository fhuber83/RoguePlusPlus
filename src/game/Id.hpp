#pragma once

namespace rogue {

/*
 * Which creature or item a link means: its slot number in game().pool (see
 * Slots), the same number a saved game gives it. A link keeps an Id instead
 * of a pointer, and the pool gives the thing (Pool::item(), creature()).
 *
 * An Id carries no generation: once its thing is discarded it names whatever
 * takes the slot next, as a kept pointer would have named a freed thing.
 * Discarding a thing drops the links to it that outlive a command (discard()
 * in list.cpp), which pool_problems() checks.
 */
template <typename T>
struct Id {
	int slot;

	friend constexpr bool operator==(const Id &, const Id &) = default;
};

struct Item;
struct Creature;
using ItemId = Id<Item>;
using CreatureId = Id<Creature>;

}  // namespace rogue
