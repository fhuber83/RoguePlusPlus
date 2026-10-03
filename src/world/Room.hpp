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

inline constexpr int MAXROOMS = 9;	/* rooms on a level */
inline constexpr int MAXPASS = 13;	/* upper limit on number of passages */

/*
 * A room of the level, or a passage (was struct room). Level::room() gives
 * the one a RoomRef names.
 */
struct Room {
	Coord pos;			/* Upper left corner */
	Coord size;			/* Size of room */
	Coord gold;			/* Where the gold is */
	int gold_value;			/* How much the gold is worth */
	RoomFlags flags;		/* Info about the room */
	int nexits;			/* Number of exits */
	std::array<Coord, 12> exits;	/* Where the exits are */

	// A corridor where a room would be, but not a maze (was isgone())
	bool is_gone() const
	{
		return flags.test(RoomFlag::Gone) && !flags.test(RoomFlag::Maze);
	}
};

}  // namespace world
}  // namespace rogue
