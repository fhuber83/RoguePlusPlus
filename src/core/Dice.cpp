#include "core/Dice.hpp"

#include <charconv>

#include "core/Random.hpp"

namespace rogue {

namespace {

std::optional<int>
parse_int(std::string_view text)
{
	int value = 0;
	const auto *end = text.data() + text.size();
	const auto [ptr, ec] = std::from_chars(text.data(), end, value);
	if (text.empty() || ec != std::errc{} || ptr != end || value < 0)
		return std::nullopt;
	return value;
}

}  // namespace

int
Dice::roll(Random &random) const
{
	return random.roll(count, sides);
}

std::optional<Dice>
Dice::parse(std::string_view text)
{
	const auto d = text.find('d');
	if (d == std::string_view::npos)
		return std::nullopt;
	const auto count = parse_int(text.substr(0, d));
	const auto sides = parse_int(text.substr(d + 1));
	if (!count || !sides)
		return std::nullopt;
	return Dice{*count, *sides};
}

std::vector<Dice>
parse_attacks(std::string_view text)
{
	std::vector<Dice> attacks;
	for (;;) {
		const auto slash = text.find('/');
		const auto dice = Dice::parse(text.substr(0, slash));
		if (!dice)
			return {};
		attacks.push_back(*dice);
		if (slash == std::string_view::npos)
			return attacks;
		text.remove_prefix(slash + 1);
	}
}

}  // namespace rogue
