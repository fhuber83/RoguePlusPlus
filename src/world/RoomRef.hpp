#pragma once

namespace rogue {

/*
 * Which of the level's rooms or passages something is in: a room's index in
 * Level::rooms or a passage's in Level::passages (was a struct room * into
 * one of them). Level::room() gives the room.
 */
struct RoomRef {
	enum class Kind : unsigned char { Room, Passage };

	Kind kind;
	int index;

	static constexpr RoomRef room(int i) { return {Kind::Room, i}; }
	static constexpr RoomRef passage(int i) { return {Kind::Passage, i}; }

	friend constexpr bool operator==(const RoomRef &, const RoomRef &) = default;
};

}  // namespace rogue
