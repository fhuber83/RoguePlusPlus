/*
 * The rogue's moves.
 *
 * move.c	1.4 (A.I. Design)	12/22/84
 */

#include "game/Movement.hpp"

#include <optional>

#include "core/Coord.hpp"
#include "core/Glyphs.hpp"
#include "core/Maybe.hpp"
#include "entities/Creature.hpp"
#include "entities/Item.hpp"
#include "entities/MonsterCatalog.hpp"
#include "game/Game.hpp"
#include "game/Messages.hpp"
#include "items/Kinds.hpp"
#include "rules/Combat.hpp"
#include "ui/Display.hpp"
#include "world/Level.hpp"
#include "world/MapFlags.hpp"
#include "world/Rooms.hpp"
#include "world/Trap.hpp"
#include "world/Traps.hpp"

namespace rogue {

namespace {

/*
 * turn_corner:
 *	A running rogue who hits a wall in a passage follows the passage if it
 *	goes on to one side only: the new direction of the run, or nullopt if
 *	he stops.
 */
std::optional<Coord>
turn_corner()
{
	rogue::Turn &turn = game().turn;
	rogue::Player &player = game().player;
	world::Level &level = game().level;
	const Coord pos = player.body.pos;

	if (!turn.running || !level.room(*player.body.room).is_gone() || player.body.is(CreatureFlag::Blind))
		return std::nullopt;
	auto opens = [&](int y, int x) {
		return level.flags_at(y, x).test(MapFlag::Passage) || level.at(y, x) == DOOR;
	};
	switch (turn.run_dir) {
	case 'h':
	case 'l': {
		const bool up = pos.y > 1 && opens(pos.y - 1, pos.x);
		const bool down = pos.y < maxrow - 1 && opens(pos.y + 1, pos.x);
		if (up == down)
			return std::nullopt;
		turn.run_dir = up ? 'k' : 'j';
		return Coord{0, up ? -1 : 1};
	}
	case 'j':
	case 'k': {
		const bool left = pos.x > 1 && opens(pos.y, pos.x - 1);
		const bool right = pos.x < MAXCOLS - 2 && opens(pos.y, pos.x + 1);
		if (left == right)
			return std::nullopt;
		turn.run_dir = left ? 'h' : 'l';
		return Coord{left ? -1 : 1, 0};
	}
	default:
		return std::nullopt;
	}
}

// Whether a square showing ch stops a move: a wall, or nothing
bool
is_bound(unsigned char ch)
{
	switch (ch) {
	case ' ':
	case VWALL:
	case HWALL:
	case ULWALL:
	case URWALL:
	case LLWALL:
	case LRWALL:
		return true;
	default:
		return false;
	}
}

}  // namespace

void
do_run(unsigned char ch)
{
	game().turn.running = true;
	game().turn.after = false;
	game().turn.run_dir = ch;
}

void
do_move(int dy, int dx)
{
	rogue::Turn &turn = game().turn;
	rogue::Player &player = game().player;
	world::Level &level = game().level;

	turn.first_move = false;
	if (turn.bailout) {
		turn.bailout = false;
		msg("the crack widens ... ");
		world::descend("");
		return;
	}
	if (player.no_move) {
		player.no_move--;
		msg("you are still stuck in the bear trap");
		return;
	}
	/*
	 * Do a confused move (maybe)
	 */
	Coord nh;
	if (player.body.is(CreatureFlag::Confused) && rnd(5) != 0)
		nh = rndmove(player.body);
	else
		nh = player.body.pos + Coord{dx, dy};

	/*
	 * Find the square he moves into. Off the map, into a wall or into
	 * nothing, a running rogue may turn a corner of a passage and try
	 * again; otherwise he stops there.
	 */
	unsigned char ch;
	MapFlags fl;
	for (;;) {
		if (!world::Level::off_map(nh)) {
			if (!level.diagonal_ok(player.body.pos, nh)) {
				turn.after = false;
				turn.running = false;
				return;
			}
			/*
			 * If you are running and the move does
			 * not get you anywhere stop running
			 */
			if (turn.running && player.body.pos == nh)
				turn.after = turn.running = false;
			fl = level.flags_at(nh);
			ch = level.seen_at(nh);
			/*
			 * When the hero is on the door do not allow him
			 * to run until he enters the room all the way
			 */
			if (level.at(player.body.pos) == DOOR && ch == FLOOR)
				turn.running = false;
			if (!fl.test(MapFlag::Real) && ch == FLOOR) {
				level.at(nh) = ch = TRAP;
				level.flags_at(nh).set(MapFlag::Real);
			} else if (player.body.is(CreatureFlag::Held) && ch != 'F') {
				msg("you are being held");
				return;
			}
			if (!is_bound(ch))
				break;
		}
		if (std::optional<Coord> dir = turn_corner()) {
			nh = player.body.pos + *dir;
			continue;
		}
		turn.after = turn.running = false;
		return;
	}

	// He steps onto nh
	auto step = [&] {
		ui::display().draw_tile(player.body.pos, level.at(player.body.pos));
		if (fl.test(MapFlag::Passage) && (level.at(player.old_pos) == DOOR
				|| level.flags_at(player.old_pos).test(MapFlag::Maze)))
			world::leave_room(nh);
		if (fl.test(MapFlag::Maze) && !level.flags_at(player.old_pos).test(MapFlag::Maze))
			world::enter_room(nh);
		player.body.pos = nh;
	};
	switch (ch) {
	case DOOR:
		turn.running = false;
		if (level.flags_at(player.body.pos).test(MapFlag::Passage))
			world::enter_room(nh);
		step();
		break;
	case TRAP:
		if (Trap trap = world::be_trapped(nh); trap == Trap::Door || trap == Trap::Teleport)
			return;
		step();
		break;
	case PASSAGE:
		step();
		break;
	case FLOOR:
		if (!fl.test(MapFlag::Real))
			world::be_trapped(player.body.pos);
		step();
		break;
	default:
		turn.running = false;
		if (is_monster(ch) || level.monster_at(nh))
			rules::fight(nh, ch, player.weapon_item(), false);
		else {
			if (ch != STAIRS)
				turn.take = ch;
			step();
		}
		break;
	}
}

Coord
rndmove(const Creature &who)
{
	const int y = who.pos.y + rnd(3) - 1;
	const int x = who.pos.x + rnd(3) - 1;
	const Coord to = {x, y};

	/*
	 * Now check to see if that's a legal move.  If not, don't move.
	 * (I.e., bump into the wall or whatever)
	 */
	if (to == who.pos)
		return to;
	if (world::Level::off_map({x, y}) || !game().level.diagonal_ok(who.pos, to))
		return who.pos;
	const unsigned char ch = game().level.seen_at({x, y});
	if (!step_ok(ch))
		return who.pos;
	if (ch == SCROLL)
		if (Maybe<Item> obj = game().level.object_at({x, y}); obj && obj->which<Scroll>() == Scroll::ScareMonster)
			return who.pos;
	return to;
}

}  // namespace rogue
