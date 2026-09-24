#pragma once

namespace rogue {

/*
 * A position or offset on the 80x25 screen grid (x = column, y = row).
 *
 * Kept a trivial aggregate: legacy code copies creatures and items, which
 * hold coords, with bcopy() (memmove).
 */
struct Coord {
	int x;
	int y;

	friend constexpr bool operator==(const Coord &, const Coord &) = default;

	constexpr Coord &operator+=(Coord other)
	{
		x += other.x;
		y += other.y;
		return *this;
	}

	constexpr Coord &operator-=(Coord other)
	{
		x -= other.x;
		y -= other.y;
		return *this;
	}
};

constexpr Coord operator+(Coord a, Coord b) { return a += b; }
constexpr Coord operator-(Coord a, Coord b) { return a -= b; }

// Squared Euclidean distance, the metric Rogue uses for ranges and lamps.
constexpr int
distance_sq(Coord a, Coord b)
{
	const Coord d = a - b;
	return d.x * d.x + d.y * d.y;
}

}  // namespace rogue
