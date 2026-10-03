/*
 * The squares of the level map: where a square is kept, whether it is on
 * the map, and what stands, lies or can be stepped on there.
 *
 * From misc.c.
 *
 * misc.c	1.4		(A.I. Design)	12/14/84
 */

#include "rogue.h"

namespace rogue::world {

int
INDEX(int y, int x)
{
	if constexpr (rogue::config::debug_checks)
		if (offmap(y, x))
			fatal("BAD INDEX {},{}\n", y, x);
	return x * (maxrow - 1) + y - 1;
}

bool
offmap(int y, int x)
{
	return y < 1 || y >= maxrow || x < 0 || x >= MAXCOLS;
}

unsigned char
winat(int y, int x)
{
	if (Maybe<Creature> tp = moat(y, x))
		return tp->t_disguise;
	return game().level.at(y, x);
}

bool
step_ok(unsigned char ch)
{
	switch (ch) {
	case ' ':
	case VWALL:
	case HWALL:
	case ULWALL:
	case URWALL:
	case LLWALL:
	case LRWALL:
		return false;
	default:
		return !is_monster(ch);
	}
}

Maybe<Item>
find_obj(int y, int x)
{
	for (Item &op : game().level.objects)
		if (op.o_pos.y == y && op.o_pos.x == x)
			return op;
	return std::nullopt;
}

}  // namespace rogue::world
