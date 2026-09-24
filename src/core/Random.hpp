#pragma once

#include <cstdint>
#include <random>

namespace rogue {

/*
 * Seedable source of all game randomness.
 *
 * The engine is std::mt19937, whose output sequence is fixed by the standard,
 * and the mapping to a range is done here rather than by a std distribution,
 * so a given seed produces the same dungeon with every compiler and library.
 */
class Random {
public:
	using Seed = std::uint32_t;

	explicit Random(Seed seed = 0) { reseed(seed); }

	void reseed(Seed seed);
	Seed seed() const { return seed_; }

	// Uniform integer in [0, range), or 0 if range < 1.
	int below(int range);

	// Sum of `count` rolls of a die with `sides` faces (each 1..sides).
	int roll(int count, int sides);

	// A seed derived from the current time, for new games.
	static Seed from_clock();

private:
	Seed seed_ = 0;
	std::mt19937 engine_;
};

}  // namespace rogue
