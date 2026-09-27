#include "core/Dice.hpp"

#include <format>

#include "core/Random.hpp"

namespace rogue {

int
Dice::roll(Random &random) const
{
	return random.roll(count, sides);
}

std::string
Attacks::to_string() const
{
	std::string text;
	for (const Dice &dice : *this)
		text += std::format("{}{}d{}", text.empty() ? "" : "/", dice.count, dice.sides);
	return text;
}

}  // namespace rogue
