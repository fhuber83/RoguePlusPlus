/*
 * The level's squares: where one is kept, and what stands, lies or can be
 * stepped on there.
 *
 * From misc.c (INDEX, winat, find_obj), monsters.c (moat) and rooms.c
 * (roomin, diag_ok).
 */

#include "world/Level.hpp"

#include "core/Config.hpp"
#include "game/Game.hpp"
#include "platform/Session.hpp"

namespace rogue::world {

int
Level::index(Coord pos)
{
	if constexpr (rogue::config::debug_checks)
		if (off_map(pos))
			platform::fatal("BAD INDEX {},{}\n", pos.y, pos.x);
	return pos.x * (maxrow - 1) + pos.y - 1;
}

unsigned char
Level::seen_at(Coord pos)
{
	if (Maybe<Creature> tp = monster_at(pos))
		return tp->disguise;
	return at(pos);
}

Maybe<Creature>
Level::monster_at(Coord pos) const
{
	for (Creature &tp : monsters)
		if (tp.pos == pos)
			return tp;
	return std::nullopt;
}

Maybe<Item>
Level::object_at(Coord pos) const
{
	for (Item &op : objects)
		if (op.pos == pos)
			return op;
	return std::nullopt;
}

bool
Level::diagonal_ok(Coord from, Coord to)
{
	if (to.x == from.x || to.y == from.y)
		return true;
	return step_ok(at(to.y, from.x)) && step_ok(at(from.y, to.x));
}

std::optional<RoomRef>
Level::room_at(Coord pos)
{
	for (int i = 0; i < MAXROOMS; i++) {
		const Room &r = rooms[i];
		if (pos.x < r.r_pos.x + r.r_max.x && r.r_pos.x <= pos.x
		 && pos.y < r.r_pos.y + r.r_max.y && r.r_pos.y <= pos.y)
			return RoomRef::room(i);
	}
	if (flags_at(pos).test(MapFlag::Passage))
		return passage_at(pos);
	return std::nullopt;
}

}  // namespace rogue::world
