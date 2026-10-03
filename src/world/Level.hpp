#pragma once

/*
 * The level the rogue is on: its map, rooms, passages, and what lies and
 * lives on it, and the questions asked of its squares.
 */

#include <array>
#include <optional>

#include "core/Coord.hpp"
#include "core/Glyphs.hpp"
#include "core/Maybe.hpp"
#include "entities/Creature.hpp"
#include "entities/Item.hpp"
#include "entities/List.hpp"
#include "world/MapFlags.hpp"
#include "world/Room.hpp"
#include "world/RoomRef.hpp"

namespace rogue::world {

struct Level {
	int depth = 1;					/* level: what level rogue is on */
	int ntraps = 0;					/* Number of traps on this level */
	int no_food = 0;				/* Number of levels without food */
	std::array<Room, MAXROOMS> rooms = {};	/* One for each room -- A level */
	std::array<Room, MAXPASS> passages = {};	/* One for each passage */
	/*
	 * What is at each square, and its MapFlags. Index them with index(),
	 * or use at()/flags_at().
	 */
	unsigned char map[(MAXLINES-3)*MAXCOLS] = {};	/* _level */
	MapFlags flags[(MAXLINES-3)*MAXCOLS] = {};	/* _flags */
	List<Item> objects;				/* lvl_obj: list of objects on this level */
	List<Creature> monsters;		/* mlist: list of monsters on the level */

	// Passages are dark rooms that are gone. The original table left the
	// 13th one lit by mistake.
	Level()
	{
		for (auto &p : passages)
			p.flags = RoomFlag::Gone | RoomFlag::Dark;
	}

	// Where a square is kept in map and flags (was INDEX())
	static int index(Coord pos);
	// Whether a square is off the map: the message and status lines, or
	// past the edge of the screen (was offmap())
	static constexpr bool off_map(Coord pos)
	{
		return pos.y < 1 || pos.y >= maxrow || pos.x < 0 || pos.x >= MAXCOLS;
	}

	// What is at a square (was chat())
	unsigned char &at(int y, int x) { return map[index({x, y})]; }
	unsigned char &at(Coord pos) { return map[index(pos)]; }
	// A square's MapFlags (was flat())
	MapFlags &flags_at(int y, int x) { return flags[index({x, y})]; }
	MapFlags &flags_at(Coord pos) { return flags[index(pos)]; }

	// What the rogue would see at a square: a monster's disguise, or the
	// map (was winat())
	unsigned char seen_at(Coord pos);
	// The monster at a square, if any (was moat())
	Maybe<Creature> monster_at(Coord pos) const;
	// The first object lying at a square, if any (was find_obj())
	Maybe<Item> object_at(Coord pos) const;
	// Whether a diagonal step from one square to the next is free of
	// walls on both sides (was diag_ok())
	bool diagonal_ok(Coord from, Coord to);

	// The room a square is in, or the passage of a passage square
	// (roomin() without its complaint)
	std::optional<RoomRef> room_at(Coord pos);
	// The room or passage a RoomRef names
	Room &room(RoomRef r) { return r.kind == RoomRef::Kind::Room ? rooms[r.index] : passages[r.index]; }
	const Room &room(RoomRef r) const
	{
		return r.kind == RoomRef::Kind::Room ? rooms[r.index] : passages[r.index];
	}
	// Whether a RoomRef names one of this level's rooms or passages
	static constexpr bool valid(RoomRef r)
	{
		return r.index >= 0 && r.index < (r.kind == RoomRef::Kind::Room ? MAXROOMS : MAXPASS);
	}
	// The passage a passage or maze square belongs to
	RoomRef passage_at(Coord pos) { return RoomRef::passage(flags_at(pos).passage()); }
};

}  // namespace rogue::world
