#pragma once

#include <optional>
#include <string_view>
#include <vector>

namespace rogue {

class Random;

/*
 * A dice expression "NdS": roll N dice with S sides each.
 */
struct Dice {
	int count;
	int sides;

	friend constexpr bool operator==(const Dice &, const Dice &) = default;

	constexpr int min() const { return count; }
	constexpr int max() const { return count * sides; }

	int roll(Random &random) const;

	// Parse "NdS" (e.g. "2d4"). Returns nullopt unless the whole text matches.
	static std::optional<Dice> parse(std::string_view text);
};

/*
 * Parse a '/'-separated attack list such as "1d2/1d5/1d5", one Dice per
 * attack. Returns an empty list if any element is malformed.
 */
std::vector<Dice> parse_attacks(std::string_view text);

}  // namespace rogue
