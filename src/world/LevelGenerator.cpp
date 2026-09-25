/*
 * new_level:
 *	Dig and draw a new level
 *
 * new_level.c	1.4 (A.I. Design) 12/13/84
 */

#include "rogue.h"

namespace rogue::world {

#define TREAS_ROOM 20	/* one chance in TREAS_ROOM for a treasure room */
#define MAXTREAS 10	/* maximum number of treasures in a treasure room */
#define MINTREAS 2	/* minimum number of treasures in a treasure room */

static void	treas_room(void);
static void	put_things(void);
static void	do_rooms(void);

void
new_level(void)
{
	int rm, i;
	Creature *tp;
	byte *fp;
	int index;
	coord stairs;
	rogue::Player &player = game().player;
	rogue::Level &level = game().level;

	player.body.t_flags.unset(ISHELD);	/* unhold when you go down just in case */
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
	setmem(level.map, ((MAXLINES-3)*MAXCOLS),' ');
	setmem(level.flags, (MAXLINES-3)*MAXCOLS, F_REAL);
	/*
	 * Free up the monsters on the last level
	 */
	for (tp = level.monsters.first(); tp != NULL; tp = level.monsters.after(tp))
		free_list(tp->t_pack);
	free_list(level.monsters);
	/*
	 * just in case we left some flytraps behind
	 */
	f_restor();
	/*
	 * Throw away stuff left on the previous level (if anything)
	 */
	free_list(level.objects);
	do_rooms();				/* Draw rooms */
	if (player.max_level > 1)
	{
		display().wipe();
	}
	status();
	do_passages();			/* Draw passages */
	level.no_food++;
	put_things();			/* Place objects (if any) */
	/*
	 * Place the staircase down.
	 */
	i = 0;
	do {
		rm = rnd_room();
	rnd_pos(&level.rooms[rm], &stairs);
	index = INDEX(stairs.y, stairs.x);
	} while (!isfloor(level.map[index]));
	level.map[index] = STAIRS;
	/*
	 * Place the traps
	 */
	if (rnd(10) < level.depth) {
		level.ntraps = rnd(level.depth / 4) + 1;
		if (level.ntraps > MAXTRAPS)
			level.ntraps = MAXTRAPS;
		i = level.ntraps;
		while (i--) {
			do {
				rm = rnd_room();
				rnd_pos(&level.rooms[rm], &stairs);
				index = INDEX(stairs.y, stairs.x);
			} while (!isfloor(level.map[index]));
			fp = &level.flags[index];
			*fp &= ~F_REAL;
			*fp |= rnd(NTRAPS);
		}
	}
	do {
		rm = rnd_room();
		rnd_pos(&level.rooms[rm], &hero);
		index = INDEX(hero.y, hero.x);
	} while (!(isfloor(level.map[index]) && (level.flags[index] & F_REAL)
				&& moat(hero.y, hero.x) == NULL));

	game().message.end = 0;
	enter_room(&hero);
	display().draw_tile(hero, PLAYER);
	bcopy(player.old_pos,hero);
	player.old_room = proom;
	if (on(player.body, SEEMONST))
		turn_see(FALSE);
}

/*
 * rnd_room:
 *	Pick a room that is really there
 */
int
rnd_room(void)
{
	int rm;

	do
	rm = rnd(MAXROOMS);
	while (!(!game().level.rooms[rm].r_flags.test(RoomFlag::Gone)||game().level.rooms[rm].r_flags.test(RoomFlag::Maze)));
	return rm;
}

/*
 * put_things:
 *	Put potions and scrolls on this level
 */
void
put_things(void)
{
	int i = 0;
	Item *cur;
	int rm;
	coord tp;
	rogue::Level &level = game().level;

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
			if ((cur = new_item()) != NULL) {
				attach(level.objects, cur);
				cur->o_hplus = cur->o_dplus = 0;
				cur->o_damage = cur->o_hurldmg = "0d0";
				cur->o_ac = 11;
				cur->o_type = ItemKind::Amulet;
				/*
				 * Put it somewhere
				 */
				do {
					rm = rnd_room();
					rnd_pos(&level.rooms[rm], &tp);
				} while (!isfloor(winat(tp.y, tp.x)));
				chat(tp.y, tp.x) = AMULET;
				bcopy(cur->o_pos,tp);
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
			cur = new_thing();
			attach(level.objects, cur);
			/*
			 * Put it somewhere
			 */
			do {
				rm = rnd_room();
				rnd_pos(&level.rooms[rm], &tp);
			} while (!isfloor(chat(tp.y, tp.x)));
			chat(tp.y, tp.x) = glyph_of(cur->o_type);
			bcopy(cur->o_pos,tp);
		}
}

/*
 * treas_room:
 *	Add a treasure room
 */
#define MAXTRIES 10	/* max number of tries to put down a monster */

static
void
treas_room(void)
{
	int nm, index;
	Creature *tp;
	Item *obj;
	rogue::Level &level = game().level;
	struct room *rp;
	int spots, num_monst;
	coord mp;

	rp = &level.rooms[rnd_room()];
	spots = (rp->r_max.y - 2) * (rp->r_max.x - 2) - MINTREAS;
	if (spots > (MAXTREAS - MINTREAS))
		spots = (MAXTREAS - MINTREAS);
	num_monst = nm = rnd(spots) + MINTREAS;
	while (nm-- && game().pool.total < MAXITEMS)
	{
		do
		{
			rnd_pos(rp, &mp);
			index = INDEX(mp.y, mp.x);
		} while (!isfloor(level.map[index]));
		obj = new_thing();
		bcopy(obj->o_pos,mp);
		attach(level.objects, obj);
		level.map[index] = glyph_of(obj->o_type);
	}

	/*
	 * fill up room with monsters from the next level down
	 */

	if ((nm = rnd(spots) + MINTREAS) < num_monst + 2)
		nm = num_monst + 2;
	spots = (rp->r_max.y - 2) * (rp->r_max.x - 2);
	if (nm > spots)
		nm = spots;
	level.depth++;
	while (nm--)
	{
		for (spots = 0; spots < MAXTRIES; spots++)
		{
			rnd_pos(rp, &mp);
			index = INDEX(mp.y, mp.x);
			if (isfloor(level.map[index]) && moat(mp.y, mp.x) == NULL)
				break;
		}
		if (spots != MAXTRIES)
		{
			if ((tp = new_creature()) != NULL)
			{
				new_monster(tp, randmonster(FALSE), &mp);
				tp->t_flags.set(ISMEAN);	/* no sloughers in THIS room */
				give_pack(tp);
			}
		}
	}
	level.depth--;
}

/*
 * Create the layout for the new level
 *
 * rooms.c	1.4 (A.I. Design)	12/16/84
 */

#define GOLDGRP 1

static void	draw_room(struct room *rp);
static void	vert( struct room *rp, int startx);
static void	horiz(struct room *rp, int starty);

/*
 * do_rooms:
 *	Create rooms and corridors with a connectivity graph
 */
void
do_rooms(void)
{
	int i, rm;
	rogue::Level &level = game().level;
	struct room *rp;
	Creature *tp;
	int left_out;
	coord top;
	coord bsze;
	coord mp;
	int endline;

	endline = maxrow + 1;

	/*
	 * bsze is the maximum room size
	 */
	bsze.x = COLS/3;
	bsze.y = endline/3;
	/*
	 * Clear things for a new level
	 */
	for (rp = level.rooms; rp < &level.rooms[MAXROOMS]; rp++)
	{
		rp->r_goldval = rp->r_nexits = 0;
		rp->r_flags.reset();
	}
	/*
	 * Put the gone rooms, if any, on the level
	 */
	left_out = rnd(4);
	for (i = 0; i < left_out; i++) {
		do
			rp = &level.rooms[(rm = rnd_room())];
		while (rp->r_flags.test(RoomFlag::Maze));
		rp->r_flags.set(RoomFlag::Gone);
		if (rm > 2 && level.depth > 10 && rnd(20) < level.depth - 9)
			rp->r_flags.set(RoomFlag::Maze);
	}
	/*
	 * dig and populate all the rooms on the level
	 */
	for (i = 0, rp = level.rooms; i < MAXROOMS; rp++, i++) {
		/*
		 * Find upper left corner of box that this room goes in
		 */
		top.x = (i%3)*bsze.x + 1;
		top.y = i/3*bsze.y;
		if (rp->r_flags.test(RoomFlag::Gone)) {
			/*
			 * If the gone room is a maze room, draw the maze and set the
			 * size equal to the maximum possible.
			 */
			if (rp->r_flags.test(RoomFlag::Maze)) {
				rp->r_pos.x = top.x;
				rp->r_pos.y = top.y;
				draw_maze(rp);
			} else {
				/*
				 * Place a gone room.  Make certain that there is a blank line
				 * for passage drawing.
				 */
				do {
					rp->r_pos.x = top.x + rnd(bsze.x-2) + 1;
					rp->r_pos.y = top.y + rnd(bsze.y-2) + 1;
					rp->r_max.x = -COLS;
					rp->r_max.x = -endline;
				} while (!(rp->r_pos.y > 0 && rp->r_pos.y < endline-1));
			}
			continue;
		}
		if (rnd(10) < (level.depth - 1))
			rp->r_flags.set(RoomFlag::Dark);
		/*
		 * Find a place and size for a random room
		 */
		do {
			rp->r_max.x = rnd(bsze.x - 4) + 4;
			rp->r_max.y = rnd(bsze.y - 4) + 4;
			rp->r_pos.x = top.x + rnd(bsze.x - rp->r_max.x);
			rp->r_pos.y = top.y + rnd(bsze.y - rp->r_max.y);
		} while (rp->r_pos.y == 0);
		draw_room(rp);
		/*
		 * Put the gold in
		 */
		if ((rnd(2) == 0) && (!game().player.saw_amulet || (level.depth >= game().player.max_level))) {
			Item *gold;

			if ((gold = new_item()) != NULL) {
				gold->o_goldval = rp->r_goldval = GOLDCALC;
				while (1) {
					byte gch;

					rnd_pos(rp, &rp->r_gold);
					gch =  chat(rp->r_gold.y, rp->r_gold.x);
					if (isfloor(gch))
						break;
				}
				bcopy(gold->o_pos,rp->r_gold);
				gold->o_flags = ISMANY;
				gold->o_group = GOLDGRP;
				gold->o_type = ItemKind::Gold;
				attach(level.objects, gold);
				chat(rp->r_gold.y, rp->r_gold.x) = GOLD;
			}
		}
		/*
		 * Put the monster in
		 */
		if (rnd(100) < (rp->r_goldval > 0 ? 80 : 25)) {
			if ((tp = new_creature()) != NULL) {
				byte mch;

				do {
					rnd_pos(rp, &mp);
					mch = winat(mp.y, mp.x);
				} while (!isfloor(mch));
				new_monster(tp, randmonster(FALSE), &mp);
				give_pack(tp);
			}
		}
	}
}

/*
 * draw_room:
 *	Draw a box around a room and lay down the floor
 */
void
draw_room(struct room *rp)
{
	int y, x;

	/*
	 * Here we draw normal rooms, one side at a time
	 */
	vert(rp, rp->r_pos.x);			/* Draw left side */
	vert(rp, rp->r_pos.x + rp->r_max.x - 1);	/* Draw right side */
	horiz(rp, rp->r_pos.y);			/* Draw top */
	horiz(rp, rp->r_pos.y + rp->r_max.y - 1);	/* Draw bottom */
	chat(rp->r_pos.y,rp->r_pos.x) = ULWALL;
	chat(rp->r_pos.y,rp->r_pos.x+rp->r_max.x - 1) = URWALL;
	chat(rp->r_pos.y+rp->r_max.y-1,rp->r_pos.x) = LLWALL;
	chat(rp->r_pos.y+rp->r_max.y-1,rp->r_pos.x+rp->r_max.x - 1) = LRWALL;
	/*
	 * Put the floor down
	 */
	for (y = rp->r_pos.y + 1; y < rp->r_pos.y + rp->r_max.y - 1; y++)
		for (x = rp->r_pos.x + 1; x < rp->r_pos.x + rp->r_max.x - 1; x++)
			chat(y, x) = FLOOR;
}

/*
 * vert:
 *	Draw a vertical line
 */
static
void
vert(struct room *rp, int startx)
{
	int y;

	for (y = rp->r_pos.y + 1; y <= rp->r_max.y + rp->r_pos.y - 1; y++)
		chat(y, startx) = VWALL;
}

/*
 * horiz:
 *	Draw a horizontal line
 */
static
void
horiz(struct room *rp, int starty)
{
	int x;

	for (x = rp->r_pos.x; x <= rp->r_pos.x + rp->r_max.x - 1; x++)
		chat(starty, x) = HWALL;
}

}  // namespace rogue::world
