#include "core/Random.hpp"

#include <chrono>

namespace rogue {

void
Random::reseed(Seed seed)
{
	seed_ = seed;
	engine_.seed(seed);
}

int
Random::below(int range)
{
	if (range < 1)
		return 0;
	// Rejection sampling: discard the lowest (2^32 mod range) values so every
	// residue is equally likely.
	const auto bound = static_cast<std::uint32_t>(range);
	const std::uint32_t threshold = -bound % bound;
	std::uint32_t value;
	do
		value = static_cast<std::uint32_t>(engine_());
	while (value < threshold);
	return static_cast<int>(value % bound);
}

int
Random::roll(int count, int sides)
{
	int total = 0;
	while (count-- > 0)
		total += below(sides) + 1;
	return total;
}

Random::Seed
Random::from_clock()
{
	const auto ticks = std::chrono::system_clock::now().time_since_epoch().count();
	return static_cast<Seed>(ticks ^ (ticks >> 32));
}

}  // namespace rogue
