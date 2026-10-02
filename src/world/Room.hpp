#pragma once

#include <array>

#include "core/Coord.hpp"
#include "core/Flags.hpp"

namespace rogue {

enum class RoomFlag : unsigned short {
	Dark = 0x0001,	/* room is dark */
	Gone = 0x0002,	/* room is gone (a corridor) */
	Maze = 0x0004,	/* room is a maze */
};
template <>
inline constexpr bool enable_flags<RoomFlag> = true;

using RoomFlags = Flags<RoomFlag>;

namespace world {

/*
 * A room of the level, or a passage (was Room). Level::room() gives
 * the one a RoomRef names.
 */
struct Room {
	Coord r_pos;			/* Upper left corner */
	Coord r_max;			/* Size of room */
	Coord r_gold;			/* Where the gold is */
	int r_goldval;			/* How much the gold is worth */
	RoomFlags r_flags;		/* Info about the room */
	int r_nexits;			/* Number of exits */
	std::array<Coord, 12> r_exit;	/* Where the exits are */

	// A corridor where a room would be, but not a maze (was isgone())
	bool is_gone() const
	{
		return r_flags.test(RoomFlag::Gone) && !r_flags.test(RoomFlag::Maze);
	}
};

}  // namespace world
}  // namespace rogue
