#pragma once

namespace rogue {

/*
 * sign:
 *	The sign of a number: -1, 0 or 1.
 */
constexpr int
sign(int n)
{
	return (n > 0) - (n < 0);
}

}  // namespace rogue
