#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

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
	static constexpr std::optional<Dice> parse(std::string_view text);
};

namespace detail {

// A number of decimal digits, nothing else
constexpr std::optional<int>
parse_count(std::string_view text)
{
	if (text.empty())
		return std::nullopt;
	int value = 0;
	for (char c : text) {
		if (c < '0' || c > '9' || value > (std::numeric_limits<int>::max() - (c - '0')) / 10)
			return std::nullopt;
		value = value * 10 + (c - '0');
	}
	return value;
}

}  // namespace detail

constexpr std::optional<Dice>
Dice::parse(std::string_view text)
{
	const auto d = text.find('d');
	if (d == std::string_view::npos)
		return std::nullopt;
	const auto count = detail::parse_count(text.substr(0, d));
	const auto sides = detail::parse_count(text.substr(d + 1));
	if (!count || !sides)
		return std::nullopt;
	return Dice{*count, *sides};
}

/*
 * What a creature or a weapon does in a fight: one Dice per attack, written
 * as a '/'-separated list such as "1d2/1d5/1d5". Up to four attacks, the
 * most a monster has; none is nothing at all, which differs from "0d0" (one
 * attack for no damage, that still swings).
 *
 * A literal converts at compile time, so a malformed one in a table does not
 * compile: `Attacks a = "2d4";`. Text read at run time goes through parse().
 */
class Attacks {
public:
	static constexpr std::size_t max_attacks = 4;

	constexpr Attacks() = default;

	template <std::size_t N>
	consteval Attacks(const char (&text)[N])
	{
		auto parsed = parse(std::string_view(text, N - 1));
		if (!parsed)
			throw "not an attack list like \"1d2/1d5\"";
		*this = *parsed;
	}

	// One attack
	constexpr explicit Attacks(Dice dice) : dice_{dice}, count_(1) {}

	// Parse a list such as "1d2/1d5". Returns nullopt if any element is
	// malformed or there are more than max_attacks.
	static constexpr std::optional<Attacks> parse(std::string_view text);

	// The list as parse() reads it, "" for none
	std::string to_string() const;

	constexpr std::size_t size() const { return count_; }
	constexpr bool empty() const { return count_ == 0; }
	constexpr const Dice *begin() const { return dice_.data(); }
	constexpr const Dice *end() const { return dice_.data() + count_; }

	friend constexpr bool operator==(const Attacks &a, const Attacks &b)
	{
		return std::equal(a.begin(), a.end(), b.begin(), b.end());
	}

private:
	std::array<Dice, max_attacks> dice_{};
	std::size_t count_ = 0;
};

constexpr std::optional<Attacks>
Attacks::parse(std::string_view text)
{
	Attacks attacks;
	for (;;) {
		const auto slash = text.find('/');
		const auto dice = Dice::parse(text.substr(0, slash));
		if (!dice || attacks.count_ == max_attacks)
			return std::nullopt;
		attacks.dice_[attacks.count_++] = *dice;
		if (slash == std::string_view::npos)
			return attacks;
		text.remove_prefix(slash + 1);
	}
}

}  // namespace rogue
