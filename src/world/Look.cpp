/*
 * What the rogue sees and finds around it: the quick glance after every
 * move, and searching for hidden doors and traps.
 *
 * From misc.c.
 *
 * misc.c	1.4		(A.I. Design)	12/14/84
 */

#include "world/Look.hpp"

#include <optional>

#include "core/Glyphs.hpp"
#include "core/Maybe.hpp"
#include "entities/Creature.hpp"
#include "entities/MonsterAI.hpp"
#include "entities/MonsterCatalog.hpp"
#include "game/Game.hpp"
#include "game/Messages.hpp"
#include "ui/Display.hpp"
#include "world/Map.hpp"
#include "world/MapFlags.hpp"
#include "world/Room.hpp"
#include "world/RoomRef.hpp"
#include "world/Traps.hpp"

namespace rogue::world {

/*
 * look:
 *	A quick glance all around the player
 */
void
look(bool wakeup)
{
	int x, y;
	unsigned char ch, pch;
	int index;
	Maybe<Creature> tp;
	rogue::Turn &turn = game().turn;
	rogue::Player &player = game().player;
	rogue::Level &level = game().level;
	std::optional<RoomRef> rp;
	int ey, ex;
	int passcount = 0;
	MapFlags pfl;
	int sy, sx, sumhero = 0, diffhero = 0;

	rp = player.body.t_room;
	index = INDEX(player.body.t_pos.y, player.body.t_pos.x);
	pfl = level.flags[index];
	pch = level.map[index];
	/*
	 * if the hero has moved
	 */
	if (!(player.old_pos == player.body.t_pos)) {
		if (!player.body.t_flags.test(CreatureFlag::Blind)) {
			for (x = player.old_pos.x - 1; x <= (player.old_pos.x + 1); x++)
				for (y = player.old_pos.y - 1; y <= (player.old_pos.y + 1); y++) {
					if ((y == player.body.t_pos.y && x == player.body.t_pos.x) || offmap(y,x))
						continue;
					ch = ui::display().tile_at({x, y});
					if (ch == FLOOR) {
						if (level.room(*player.old_room).r_flags.test(RoomFlag::Dark) && !level.room(*player.old_room).r_flags.test(RoomFlag::Gone))
							ui::display().draw_tile({x, y}, ' ');
					} else {
						MapFlags &fp = level.flags[INDEX(y,x)];
						/*
						 * if the maze or passage (that the hero is in!!)
						 * needs to be redrawn (passages once draw always
						 * stay on) do it now.
						 */
						if ((fp.test(MapFlag::Maze) || fp.test(MapFlag::Passage)) && (ch!=PASSAGE)
							&& (ch != STAIRS) &&
							(fp.passage() == pfl.passage()) )
								ui::display().draw_tile({x, y}, PASSAGE);
					}
				}
		}
		player.old_pos = player.body.t_pos;
		player.old_room = rp;
	}
	ey = player.body.t_pos.y + 1;
	ex = player.body.t_pos.x + 1;
	sx = player.body.t_pos.x - 1;
	sy = player.body.t_pos.y - 1;
	if (turn.door_stop && !turn.first_move && turn.running) {
		sumhero = player.body.t_pos.y + player.body.t_pos.x;
		diffhero = player.body.t_pos.y - player.body.t_pos.x;
	}
	for (y = sy; y <= ey; y++)
		if (y > 0 && y < maxrow) for (x = sx; x <= ex; x++) {
			if (x <= 0 || x >= MAXCOLS)
				continue;
			if (!player.body.t_flags.test(CreatureFlag::Blind)) {
				if (y == player.body.t_pos.y && x == player.body.t_pos.x)
					continue;
			} else if (y != player.body.t_pos.y || x != player.body.t_pos.x)
				continue;

			index = INDEX(y, x);
			/*
			 * THIS REPLICATES THE moat() MACRO.  IF MOAT IS CHANGED,
			 * THIS MUST BE CHANGED ALSO ?? What does this really mean ??
			 */
			MapFlags &fp = level.flags[index];
			ch = level.map[index];
			/*
			 * No Doors
			 */
			if (pch != DOOR && ch != DOOR) {
				/*
				 * Either hero or other in a passage
				 */
				if (pfl.test(MapFlag::Passage) != fp.test(MapFlag::Passage)) {
					/*
					 * Neither is in a maze
					 */
					if ( ! pfl.test(MapFlag::Maze) && ! fp.test(MapFlag::Maze))
						continue;
				}
				/*
				 * Not in same passage
				 */
				else if (fp.test(MapFlag::Passage) && fp.passage() != pfl.passage())
					continue;
			}

			if ((tp = entities::moat(y,x))) {
				if (player.body.t_flags.test(CreatureFlag::SeeMonst) && tp->t_flags.test(CreatureFlag::Invisible)) {
					if (turn.door_stop && !turn.first_move)
						turn.running = false;
					continue;
				} else {
					if (wakeup)
						entities::wake_monster(y, x);
					if (tp->t_oldch != ' ' ||
						(!level.room(*rp).r_flags.test(RoomFlag::Dark) && !player.body.t_flags.test(CreatureFlag::Blind)))
							tp->t_oldch = level.map[index];
					if (entities::see_monst(*tp))
						ch = tp->t_disguise;
				}
			}

			/*
			 * The current character used for IBM ARMOR doesn't
			 * look right in Inverse
			 */
			ui::display().draw_tile({x, y}, ch,
					((ch!=PASSAGE) && fp.test(MapFlag::Passage | MapFlag::Maze) && ch != ARMOR)
						? ui::TileStyle::Inverse : ui::TileStyle::Normal);

			if (turn.door_stop && !turn.first_move && turn.running) {
				switch (turn.run_dir) {
				case 'h':
					if (x == ex)
						continue;
					break;
				case 'j':
					if (y == sy)
						continue;
					break;
				case 'k':
					if (y == ey)
						continue;
					break;
				case 'l':
					if (x == sx)
						continue;
					break;
				case 'y':
					if ((y + x) - sumhero >= 1)
						continue;
					break;
				case 'u':
					if ((y - x) - diffhero >= 1)
						continue;
					break;
				case 'n':
					if ((y + x) - sumhero <= -1)
						continue;
					break;
				case 'b':
					if ((y - x) - diffhero <= -1)
						continue;
					break;
				}
				switch (ch) {
				case DOOR:
					if (x == player.body.t_pos.x || y == player.body.t_pos.y)
						turn.running = false;
					break;
				case PASSAGE:
					if (x == player.body.t_pos.x || y == player.body.t_pos.y)
						passcount++;
					break;
				case FLOOR:
				case VWALL:
				case HWALL:
				case ULWALL:
				case URWALL:
				case LLWALL:
				case LRWALL:
				case ' ':
					break;
				default:
					turn.running = false;
					break;
				}
			}
		}
	if (turn.door_stop && !turn.first_move && passcount > 1)
		turn.running = false;
	ui::display().draw_tile(player.body.t_pos, PLAYER,
			(level.flags_at(player.body.t_pos).test(MapFlag::Passage) || (player.was_trapped == rogue::Trapped::Teleported)
					|| level.flags_at(player.body.t_pos).test(MapFlag::Maze))
				? ui::TileStyle::Inverse : ui::TileStyle::Normal);
	if (player.was_trapped != rogue::Trapped::None) {
		ui::display().bell();
		player.was_trapped = rogue::Trapped::None;
	}
}

/*
 * search:
 *	Player gropes about him to find hidden things.
 */
void
search()
{
	int y, x;
	int ey, ex;
	rogue::Player &player = game().player;
	rogue::Level &level = game().level;

	if (player.body.t_flags.test(CreatureFlag::Blind))
		return;
	ey = player.body.t_pos.y + 1;
	ex = player.body.t_pos.x + 1;
	for (y = player.body.t_pos.y - 1; y <= ey; y++)
		for (x = player.body.t_pos.x - 1; x <= ex; x++)
		{
			if ((y == player.body.t_pos.y && x == player.body.t_pos.x) || offmap(y, x))
				continue;
			MapFlags &fp = level.flags_at(y, x);
			if (!fp.test(MapFlag::Real))
				switch (level.at(y, x))
				{
					case VWALL:
					case HWALL:
					case ULWALL:
					case URWALL:
					case LLWALL:
					case LRWALL:
						if (rnd(5) != 0)
							break;
						level.at(y, x) = DOOR;
						fp.set(MapFlag::Real);
						game().turn.count = game().turn.running = false;
						break;
					case FLOOR:
						if (rnd(2) != 0)
							break;
						level.at(y, x) = TRAP;
						fp.set(MapFlag::Real);
						game().turn.count = game().turn.running = false;
						msg("you found {}", tr_name(fp.trap()));
						break;
				}
		}
}

}  // namespace rogue::world
