/*
 * Rooms at play time: entering and leaving one, which room a square is
 * in, and what the rogue can see from where it stands.
 *
 * rnd_pos(), enter_room() and leave_room() come from rooms.c; roomin() and
 * cansee() from chase.c; door_open() from move.c.
 *
 * rooms.c	1.4 (A.I. Design)	12/16/84
 * chase.c	1.32	(A.I. Design) 12/12/84
 * move.c	1.4 (A.I. Design)	12/22/84
 */

#include "world/Rooms.hpp"

#include <optional>

#include "core/Config.hpp"
#include "core/Coord.hpp"
#include "core/Glyphs.hpp"
#include "core/Maybe.hpp"
#include "entities/Creature.hpp"
#include "entities/MonsterAI.hpp"
#include "entities/MonsterCatalog.hpp"
#include "game/Game.hpp"
#include "game/Keyboard.hpp"
#include "game/Messages.hpp"
#include "rules/Scheduler.hpp"
#include "ui/Display.hpp"
#include "world/Level.hpp"
#include "world/LevelGenerator.hpp"
#include "world/Look.hpp"
#include "world/Room.hpp"
#include "world/RoomRef.hpp"

namespace rogue::world {

namespace {

/*
 * door_open:
 *	Called to illuminate a room.  If it is dark, remove anything
 *	that might move.
 */
void
door_open(const Room &rp)
{
	if (!rp.flags.test(RoomFlag::Gone) && !game().player.body.is(CreatureFlag::Blind))
		for (int j = rp.pos.y; j < rp.pos.y + rp.size.y; j++)
			for (int k = rp.pos.x; k < rp.pos.x + rp.size.x; k++) {
				unsigned char ch = game().level.seen_at({k, j});
				/* move(j, k); Why do this,?????? */
				if (is_monster(ch)) {
					Maybe<Creature> tp = entities::wake_monster(j, k);
					if (!tp)
					{
						continue;
					}
					if (tp->under == ' ' && !rp.flags.test(RoomFlag::Dark)
						&& !game().player.body.is(CreatureFlag::Blind))
							tp->under = game().level.at(j, k);
				}
			}
}

}  // namespace

/*
 * roomin:
 *	Find	what room some coordinates are in. nullopt	means they aren't
 *	in any room.
 */
std::optional<RoomRef>
roomin(Coord cp)
{
	std::optional<RoomRef> r = game().level.room_at(cp);
	if (!r) {
		if constexpr (rogue::config::debug_checks)
			debug("in some bizarre place ({}, {})", cp.y, cp.x);
		game().turn.bailout = true;
	}
	return r;
}

/*
 * cansee:
 *	Returns true	if the hero can	see a certain coordinate.
 */
bool
cansee(int y, int x)
{
	rogue::Player &player = game().player;

	if (player.body.is(CreatureFlag::Blind))
		return	false;
	if (distance_sq({x, y}, player.body.pos) < LAMPDIST)
		return	true;
	/*
	 * We can only see if the hero in the same room as
	 * the coordinate and the room is lit or if	it is close.
	 */
	std::optional<RoomRef> rer = roomin({x, y});
	return (rer	== player.body.room && !game().level.room(*rer).flags.test(RoomFlag::Dark));
}

/*
 * rnd_pos:
 *	Pick a random spot in a room
 */
Coord
rnd_pos(const Room &rp)
{
	// a braced list is evaluated in order: x is drawn before y
	return Coord{rp.pos.x + rnd(rp.size.x - 2) + 1, rp.pos.y + rnd(rp.size.y - 2) + 1};
}

/*
 * enter_room:
 *	Code that is executed whenver you appear in a room
 */
void
enter_room(Coord cp)
{
	world::Level &level = game().level;

	const std::optional<RoomRef> in = game().player.body.room = roomin(cp);
	// roomin() sets bailout when it finds no room
	if (game().turn.bailout || (level.room(*in).flags.test(RoomFlag::Gone) && !level.room(*in).flags.test(RoomFlag::Maze))) {
		if constexpr (rogue::config::debug_checks)
			debug("in a gone room");
		return;
	}
	const Room &rp = level.room(*in);
	door_open(rp);
	if (!rp.flags.test(RoomFlag::Dark) && !game().player.body.is(CreatureFlag::Blind) && !rp.flags.test(RoomFlag::Maze))
		for (int y = rp.pos.y; y < rp.size.y + rp.pos.y; y++) {
			for (int x = rp.pos.x; x < rp.size.x + rp.pos.x; x++) {
				/*
				 * Displaying monsters is all handled in the
				 * chase code now
				 */
				Maybe<Creature> tp = level.monster_at({x, y});
				if (!tp || !entities::see_monst(*tp))
					ui::display().draw_tile({x, y}, level.at(y, x));
				else {
					tp->under = level.at(y, x);
					ui::display().draw_tile({x, y}, tp->disguise);
				}
			}
		}
}

/*
 * leave_room:
 *	Code for when we exit a room
 */
void
leave_room(Coord cp)
{
	rogue::Player &player = game().player;

	const Room &rp = game().level.room(*player.body.room);
	player.body.room = game().level.passage_at(cp);
	unsigned char floor = (rp.flags.test(RoomFlag::Dark) && !player.body.is(CreatureFlag::Blind)) ? ' ' : FLOOR;
	if (rp.flags.test(RoomFlag::Maze))
		floor = PASSAGE;
	for (int y = rp.pos.y + 1; y < rp.size.y + rp.pos.y - 1; y++)
		for (int x = rp.pos.x + 1; x < rp.size.x + rp.pos.x - 1; x++)
			switch (unsigned char ch = ui::display().tile_at({x, y})) {
			case ' ':
			case PASSAGE:
			case TRAP:
			case STAIRS:
				break;
			case FLOOR:
				if (floor == ' ')
					ui::display().draw_tile({x, y}, ' ');
				break;
			default:
				/*
				 * to check for monster, we have to strip out
				 * standout bit (the glyph has none)
				 */
				if (is_monster(ch))
				{
					if (player.body.is(CreatureFlag::SeeMonst)) {
						ui::display().draw_tile({x, y}, ch, ui::TileStyle::Inverse);
						break;
					} else
						game().level.monster_at({x, y})->under = '@';
				}
				ui::display().draw_tile({x, y}, floor);
				break;
			}
	door_open(rp);
}

/*
 * teleport comes from wizard.c (wizard.c	1.4 (AI Design)	12/14/84).
 */

/*
 * teleport:
 *	Bamf the hero someplace else
 */
void
teleport()
{
	rogue::Player &player = game().player;

	ui::display().draw_tile(player.body.pos, game().level.at(player.body.pos));
	int rm;
	Coord c;
	do
	{
		rm = rnd_room();
		c = rnd_pos(game().level.rooms[rm]);
	} while (!(step_ok(game().level.seen_at(c))));
	if (RoomRef::room(rm) != player.body.room)
	{
		leave_room(player.body.pos);
		player.body.pos = c;
		enter_room(player.body.pos);
	}
	else
	{
		player.body.pos = c;
		look(true);
	}
	ui::display().draw_tile(player.body.pos, PLAYER);
	/*
	 * turn off Held in case teleportation was done while fighting
	 * a Fungi
	 */
	if (player.body.is(CreatureFlag::Held)) {
		player.body.flags.unset(CreatureFlag::Held);
		entities::f_restor();
	}
	player.no_move = 0;
	game().turn.count = 0;
	game().turn.running = false;
	flush_type();
	/*
	 * Teleportation can be a confusing experience
	 */
	if (player.body.is(CreatureFlag::Confused))
		rules::lengthen(rules::Event::Unconfuse, rnd(4)+2);
	else
		rules::fuse(rules::Event::Unconfuse, rnd(4)+2);
	player.body.flags.set(CreatureFlag::Confused);
}

}  // namespace rogue::world
