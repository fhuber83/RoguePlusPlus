/*
 * new_level:
 *	Dig and draw a new level
 *
 * new_level.c	1.4 (A.I. Design) 12/13/84
 */

#include "world/LevelGenerator.hpp"

#include <algorithm>

#include "core/Coord.hpp"
#include "core/Glyphs.hpp"
#include "core/Maybe.hpp"
#include "entities/Creature.hpp"
#include "entities/Item.hpp"
#include "entities/MonsterCatalog.hpp"
#include "game/Game.hpp"
#include "game/Pool.hpp"
#include "game/StatusLine.hpp"
#include "items/ItemCatalog.hpp"
#include "items/effects/Potion.hpp"
#include "ui/Display.hpp"
#include "world/Level.hpp"
#include "world/MapFlags.hpp"
#include "world/Maze.hpp"
#include "world/Passages.hpp"
#include "world/Room.hpp"
#include "world/Rooms.hpp"
#include "world/Trap.hpp"

namespace rogue::world {

namespace {

constexpr int MAXOBJ = 9;	/* tries to put a thing on a level */
constexpr int MAXTRAPS = 10;	/* traps on a level at most */

}  // namespace

constexpr int TREAS_ROOM = 20;	/* one chance in TREAS_ROOM for a treasure room */
constexpr int MAXTREAS = 10;	/* maximum number of treasures in a treasure room */
constexpr int MINTREAS = 2;	/* minimum number of treasures in a treasure room */

namespace {

void	treas_room();
void	put_things();
void	do_rooms();

}  // namespace

void
new_level()
{
	rogue::Player &player = game().player;
	world::Level &level = game().level;

	player.body.flags.unset(CreatureFlag::Held);	/* unhold when you go down just in case */
	/*
	 * Monsters only get displayed when you move
	 * so start a level by having the poor guy rest
	 * God forbid he lands next to a monster!
	 */
	if (level.depth > player.max_level)
		player.max_level = level.depth;
	/*
	 * Clean things off from last level
	 */
	std::ranges::fill(level.map, ' ');
	std::ranges::fill(level.flags, MapFlags(MapFlag::Real));
	/*
	 * Free up the monsters on the last level
	 */
	for (Creature &tp : level.monsters)
		list_free(tp.pack);
	list_free(level.monsters);
	/*
	 * just in case we left some flytraps behind
	 */
	entities::f_restor();
	/*
	 * Throw away stuff left on the previous level (if anything)
	 */
	list_free(level.objects);
	do_rooms();				/* Draw rooms */
	if (player.max_level > 1)
	{
		ui::display().wipe();
	}
	status();
	do_passages();			/* Draw passages */
	level.no_food++;
	put_things();			/* Place objects (if any) */
	/*
	 * Place the staircase down.
	 */
	Coord stairs;
	int index;
	do {
		int rm = rnd_room();
		stairs = rnd_pos(level.rooms[rm]);
		index = Level::index(stairs);
	} while (!is_floor(level.map[index]));
	level.map[index] = STAIRS;
	/*
	 * Place the traps
	 */
	if (rnd(10) < level.depth) {
		level.ntraps = rnd(level.depth / 4) + 1;
		if (level.ntraps > MAXTRAPS)
			level.ntraps = MAXTRAPS;
		int i = level.ntraps;
		while (i--) {
			do {
				int rm = rnd_room();
				stairs = rnd_pos(level.rooms[rm]);
				index = Level::index(stairs);
			} while (!is_floor(level.map[index]));
			MapFlags &fp = level.flags[index];
			fp.unset(MapFlag::Real);
			fp.set_trap(static_cast<Trap>(rnd(kind_count<Trap>)));
		}
	}
	do {
		int rm = rnd_room();
		player.body.pos = rnd_pos(level.rooms[rm]);
		index = Level::index(player.body.pos);
	} while (!(is_floor(level.map[index]) && level.flags[index].test(MapFlag::Real)
				&& !level.monster_at(player.body.pos)));

	game().message.end = 0;
	enter_room(player.body.pos);
	ui::display().draw_tile(player.body.pos, PLAYER);
	player.old_pos = player.body.pos;
	player.old_room = player.body.room;
	if (player.body.flags.test(CreatureFlag::SeeMonst))
		items::effects::turn_see(false);
}

/*
 * rnd_room:
 *	Pick a room that is really there
 */
int
rnd_room()
{
	int rm;

	do
	rm = rnd(MAXROOMS);
	while (!(!game().level.rooms[rm].r_flags.test(RoomFlag::Gone)||game().level.rooms[rm].r_flags.test(RoomFlag::Maze)));
	return rm;
}

namespace {

/*
 * put_things:
 *	Put potions and scrolls on this level
 */
void
put_things()
{
	int i = 0;
	world::Level &level = game().level;

	/*
	 * Once you have found the amulet, the only way to get new stuff is
	 * go down into the dungeon.
	 * This is real unfair - I'm going to allow one thing, that way
	 * the poor guy will get some food.
	 */
	if (game().player.saw_amulet && level.depth < game().player.max_level)
		i = MAXOBJ - 1;
	else {
		/*
		 * If he is really deep in the dungeon and he hasn't found the
		 * amulet yet, put it somewhere on the ground
		 * Check this first so if we are out of memory the guy has a
		 * hope of getting the amulet
		 */
		if (level.depth >= AMULETLEVEL && !game().player.saw_amulet) {
			if (Maybe<Item> cur = new_item()) {
				level.objects.push_front(*cur);
				cur->o_hplus = cur->o_dplus = 0;
				cur->o_damage = cur->o_hurldmg = "0d0";
				cur->o_ac = 11;
				cur->o_type = ItemKind::Amulet;
				/*
				 * Put it somewhere
				 */
				Coord tp;
				do {
					int rm = rnd_room();
					tp = rnd_pos(level.rooms[rm]);
				} while (!is_floor(level.seen_at(tp)));
				level.at(tp) = AMULET;
				cur->o_pos = tp;
			}
		}
		/*
		 * check for treasure rooms, and if so, put it in.
		 */
		if (rnd(TREAS_ROOM) == 0)
			treas_room();
	}
	/*
	 * Do MAXOBJ attempts to put things on a level
	 */
	for (;i < MAXOBJ; i++)
		if (game().pool.total < MAXITEMS && rnd(100) < 35) {
			/*
			 * Pick a new object and link it in the list
			 */
			Maybe<Item> cur = items::new_thing();
			level.objects.push_front(*cur);
			/*
			 * Put it somewhere
			 */
			Coord tp;
			do {
				int rm = rnd_room();
				tp = rnd_pos(level.rooms[rm]);
			} while (!is_floor(level.at(tp)));
			level.at(tp) = glyph_of(cur->o_type);
			cur->o_pos = tp;
		}
}

}  // namespace

/*
 * treas_room:
 *	Add a treasure room
 */
constexpr int MAXTRIES = 10;	/* max number of tries to put down a monster */

namespace {

void
treas_room()
{
	world::Level &level = game().level;

	const Room &rp = level.rooms[rnd_room()];
	int spots = (rp.r_max.y - 2) * (rp.r_max.x - 2) - MINTREAS;
	if (spots > (MAXTREAS - MINTREAS))
		spots = (MAXTREAS - MINTREAS);
	int num_monst = rnd(spots) + MINTREAS;
	int nm = num_monst;
	while (nm-- && game().pool.total < MAXITEMS)
	{
		Coord mp;
		int index;
		do
		{
			mp = rnd_pos(rp);
			index = Level::index(mp);
		} while (!is_floor(level.map[index]));
		Maybe<Item> obj = items::new_thing();
		obj->o_pos = mp;
		level.objects.push_front(*obj);
		level.map[index] = glyph_of(obj->o_type);
	}

	/*
	 * fill up room with monsters from the next level down
	 */

	if ((nm = rnd(spots) + MINTREAS) < num_monst + 2)
		nm = num_monst + 2;
	spots = (rp.r_max.y - 2) * (rp.r_max.x - 2);
	if (nm > spots)
		nm = spots;
	level.depth++;
	while (nm--)
	{
		Coord mp;
		for (spots = 0; spots < MAXTRIES; spots++)
		{
			mp = rnd_pos(rp);
			int index = Level::index(mp);
			if (is_floor(level.map[index]) && !level.monster_at(mp))
				break;
		}
		if (spots != MAXTRIES)
		{
			if (Maybe<Creature> tp = new_creature())
			{
				entities::new_monster(*tp, entities::randmonster(false), mp);
				tp->flags.set(CreatureFlag::Mean);	/* no sloughers in THIS room */
				entities::give_pack(*tp);
			}
		}
	}
	level.depth--;
}

}  // namespace

/*
 * Create the layout for the new level
 *
 * rooms.c	1.4 (A.I. Design)	12/16/84
 */

constexpr int GOLDGRP = 1;

namespace {

void	draw_room(const Room &rp);
void	vert(const Room &rp, int startx);
void	horiz(const Room &rp, int starty);

/*
 * do_rooms:
 *	Create rooms and corridors with a connectivity graph
 */
void
do_rooms()
{
	world::Level &level = game().level;
	int endline = maxrow + 1;

	/*
	 * bsze is the maximum room size
	 */
	Coord bsze = {MAXCOLS/3, endline/3};
	/*
	 * Clear things for a new level
	 */
	for (Room &rp : level.rooms)
	{
		rp.r_goldval = rp.r_nexits = 0;
		rp.r_flags.reset();
	}
	/*
	 * Put the gone rooms, if any, on the level
	 */
	int left_out = rnd(4);
	for (int i = 0; i < left_out; i++) {
		int rm;
		do
			rm = rnd_room();
		while (level.rooms[rm].r_flags.test(RoomFlag::Maze));
		Room &rp = level.rooms[rm];
		rp.r_flags.set(RoomFlag::Gone);
		if (rm > 2 && level.depth > 10 && rnd(20) < level.depth - 9)
			rp.r_flags.set(RoomFlag::Maze);
	}
	/*
	 * dig and populate all the rooms on the level
	 */
	for (int i = 0; i < MAXROOMS; i++) {
		Room &rp = level.rooms[i];

		/*
		 * Find upper left corner of box that this room goes in
		 */
		Coord top = {(i%3)*bsze.x + 1, i/3*bsze.y};
		if (rp.r_flags.test(RoomFlag::Gone)) {
			/*
			 * If the gone room is a maze room, draw the maze and set the
			 * size equal to the maximum possible.
			 */
			if (rp.r_flags.test(RoomFlag::Maze)) {
				rp.r_pos.x = top.x;
				rp.r_pos.y = top.y;
				draw_maze(rp);
			} else {
				/*
				 * Place a gone room.  Make certain that there is a blank line
				 * for passage drawing.
				 */
				do {
					rp.r_pos.x = top.x + rnd(bsze.x-2) + 1;
					rp.r_pos.y = top.y + rnd(bsze.y-2) + 1;
					rp.r_max.x = -MAXCOLS;
					rp.r_max.x = -endline;
				} while (!(rp.r_pos.y > 0 && rp.r_pos.y < endline-1));
			}
			continue;
		}
		if (rnd(10) < (level.depth - 1))
			rp.r_flags.set(RoomFlag::Dark);
		/*
		 * Find a place and size for a random room
		 */
		do {
			rp.r_max.x = rnd(bsze.x - 4) + 4;
			rp.r_max.y = rnd(bsze.y - 4) + 4;
			rp.r_pos.x = top.x + rnd(bsze.x - rp.r_max.x);
			rp.r_pos.y = top.y + rnd(bsze.y - rp.r_max.y);
		} while (rp.r_pos.y == 0);
		draw_room(rp);
		/*
		 * Put the gold in
		 */
		if ((rnd(2) == 0) && (!game().player.saw_amulet || (level.depth >= game().player.max_level))) {
			if (Maybe<Item> gold = new_item()) {
				gold->gold_value() = rp.r_goldval = gold_calc();
				while (1) {
					rp.r_gold = rnd_pos(rp);
					unsigned char gch = level.at(rp.r_gold);
					if (is_floor(gch))
						break;
				}
				gold->o_pos = rp.r_gold;
				gold->o_flags = ItemFlag::Many;
				gold->o_group = GOLDGRP;
				gold->o_type = ItemKind::Gold;
				level.objects.push_front(*gold);
				level.at(rp.r_gold) = GOLD;
			}
		}
		/*
		 * Put the monster in
		 */
		if (rnd(100) < (rp.r_goldval > 0 ? 80 : 25)) {
			if (Maybe<Creature> tp = new_creature()) {
				Coord mp;
				unsigned char mch;
				do {
					mp = rnd_pos(rp);
					mch = level.seen_at(mp);
				} while (!is_floor(mch));
				entities::new_monster(*tp, entities::randmonster(false), mp);
				entities::give_pack(*tp);
			}
		}
	}
}

/*
 * draw_room:
 *	Draw a box around a room and lay down the floor
 */
void
draw_room(const Room &rp)
{
	world::Level &level = game().level;

	/*
	 * Here we draw normal rooms, one side at a time
	 */
	vert(rp, rp.r_pos.x);			/* Draw left side */
	vert(rp, rp.r_pos.x + rp.r_max.x - 1);	/* Draw right side */
	horiz(rp, rp.r_pos.y);			/* Draw top */
	horiz(rp, rp.r_pos.y + rp.r_max.y - 1);	/* Draw bottom */
	level.at(rp.r_pos) = ULWALL;
	level.at(rp.r_pos.y, rp.r_pos.x+rp.r_max.x - 1) = URWALL;
	level.at(rp.r_pos.y+rp.r_max.y-1, rp.r_pos.x) = LLWALL;
	level.at(rp.r_pos.y+rp.r_max.y-1, rp.r_pos.x+rp.r_max.x - 1) = LRWALL;
	/*
	 * Put the floor down
	 */
	for (int y = rp.r_pos.y + 1; y < rp.r_pos.y + rp.r_max.y - 1; y++)
		for (int x = rp.r_pos.x + 1; x < rp.r_pos.x + rp.r_max.x - 1; x++)
			level.at(y, x) = FLOOR;
}

/*
 * vert:
 *	Draw a vertical line
 */
void
vert(const Room &rp, int startx)
{
	for (int y = rp.r_pos.y + 1; y <= rp.r_max.y + rp.r_pos.y - 1; y++)
		game().level.at(y, startx) = VWALL;
}

/*
 * horiz:
 *	Draw a horizontal line
 */
void
horiz(const Room &rp, int starty)
{
	for (int x = rp.r_pos.x; x <= rp.r_pos.x + rp.r_max.x - 1; x++)
		game().level.at(starty, x) = HWALL;
}

}  // namespace

}  // namespace rogue::world
