/*
 * Code	for one	creature to chase another
 *
 * chase.c	1.32	(A.I. Design) 12/12/84
 */

#include "rogue.h"

namespace rogue::entities {

static void	do_chase(Creature &th);
static void	chase(Creature &tp, Coord ee);

constexpr int DRAGONSHOT = 5;	/* one chance in DRAGONSHOT that a dragon will flame */

static coord ch_ret;			/* Where chasing takes	you */

/*
 * runners:
 *	Make all the running monsters move.
 */
void
runners()
{
	Maybe<Creature> tp;
	int dist;
	rogue::Player &player = game().player;

	for (tp = game().level.monsters.first(); tp; tp = game().level.monsters.after(*tp)) {
		if (!tp->t_flags.test(ISHELD) && tp->t_flags.test(ISRUN)) {
			const CreatureId id = *game().pool.id_of(*tp);
			dist = distance_sq(player.body.t_pos, tp->t_pos);
			if	(!(tp->t_flags.test(ISSLOW) || (tp->t_type == 'S' && dist > 3)) || tp->t_turn)
				do_chase(*tp);
			/*
			 * do_chase() can end in attack(), which removes tp from the
			 * level (a Leprechaun or Nymph vanishes once it steals). Once
			 * that happens tp is a freed pool slot and must not be read or
			 * even dereferenced again, so it is looked for by its Id; the
			 * walk stops at this point, as in the original, where the
			 * detached entry had no next (see MODERNIZATION.md 6.4).
			 */
			if (!game().level.monsters.contains(id))
				break;
			if (tp->t_flags.test(ISHASTE))
				do_chase(*tp);
			if (!game().level.monsters.contains(id))
				break;
			dist = distance_sq(player.body.t_pos, tp->t_pos);
			if (tp->t_flags.test(ISFLY) && dist > 3)
				do_chase(*tp);
			if (!game().level.monsters.contains(id))
				break;
			tp->t_turn ^= true;
		}
	}
}

/*
 * do_chase:
 *	Make one thing chase another.
 */
static void
do_chase(Creature &th)
{
	int	mindist	= 32767, i, dist;
	bool door;
	Maybe<Item> obj;
	std::optional<RoomRef> oroom;
	std::optional<RoomRef> rer, ree;	/* room of chaser, room of chasee */
	coord target;				/* Temporary	destination for	chaser */
	rogue::Player &player = game().player;
	rogue::Level &level = game().level;

	rer	= th.t_room;		/* Find room of chaser */
	if (th.t_flags.test(ISGREED) && level.room(*rer).r_goldval == 0)
		th.t_dest = Hero{};	/*	If gold	has been taken,	run after hero */
	ree	= player.body.t_room;
	if (th.t_dest != Destination(Hero{}))	/*	Find room of chasee */
		ree = roomin(game().where(*th.t_dest));
	if (!ree)
		return;
	/*
	 * We don't	count doors as inside rooms for	this routine
	 */
	door = (level.at(th.t_pos) == DOOR);


	/*
	 * If the object of	our desire is in a different room,
	 * and we are not in a maze, run to	the door nearest to
	 * our goal.
	 */
over:
	if (rer != ree && !level.room(*rer).r_flags.test(RoomFlag::Maze))
	{
		const struct room &from = level.room(*rer);
		const Coord dest = game().where(*th.t_dest);

		for (i	= 0; i < from.r_nexits;	i++) {	/*	loop through doors */
			dist = distance_sq(dest, from.r_exit[i]);
			if	(dist <	mindist) {
				target = from.r_exit[i];
				mindist = dist;
			}
		}
		if (door) {
			rer = level.passage_at(th.t_pos);
			door = false;
			goto over;
		}
	} else {
		target =	game().where(*th.t_dest);
		/*
		 * For	monsters which can fire	bolts at the poor hero,	we check to
		 * see	if (a) the hero	in on a	straight line from it, and (b) that
		 * it is within shooting distance, but	outside	of striking range.
		 */
		if ((th.t_type == 'D' || th.t_type == 'I')
			&&	(th.t_pos.y ==	player.body.t_pos.y || th.t_pos.x == player.body.t_pos.x
			 || abs(th.t_pos.y - player.body.t_pos.y) == abs(th.t_pos.x - player.body.t_pos.x))
			&&	((dist=distance_sq(th.t_pos, player.body.t_pos)) > 2
			 && dist <= BOLT_LENGTH	* BOLT_LENGTH)
			&&	!th.t_flags.test(ISCANC) && rnd(DRAGONSHOT) == 0)
		{
			game().turn.running = false;
			game().turn.delta.y = sign(player.body.t_pos.y - th.t_pos.y);
			game().turn.delta.x = sign(player.body.t_pos.x - th.t_pos.x);
			fire_bolt(th.t_pos, game().turn.delta, th.t_type == 'D' ? "flame" : "frost");
			return;
		}
	}
	/*
	 * This now	contains what we want to run to	this time
	 * so we run to it.	 If we hit it we either	want to	fight it
	 * or stop running
	 */
	chase(th, target);
	if (ch_ret == player.body.t_pos) {
		attack(th);
		return;
	} else if (ch_ret == game().where(*th.t_dest)) {
		for (obj = level.objects.first(); obj; obj = level.objects.after(*obj))
			if	(th.t_dest == Destination(*game().pool.id_of(obj))) {
				unsigned char oldchar;

				level.objects.remove(*obj);
				th.t_pack.push_front(*obj);
				oldchar = level.at(obj->o_pos) =
				level.room(*th.t_room).r_flags.test(RoomFlag::Gone) ? PASSAGE : FLOOR;
				if (cansee(obj->o_pos.y, obj->o_pos.x))
					display().draw_tile(obj->o_pos, oldchar);
				th.t_dest = find_dest(th);
				break;
			}
	}
	if (th.t_type == 'F')
		return;
	/*
	 * If the chasing thing moved, update the screen
	 */
	if (th.t_oldch != '@') {
		if	(th.t_oldch ==	' ' && cansee(th.t_pos.y, th.t_pos.x)
			   && level.map[INDEX(th.t_pos.y,th.t_pos.x)] == FLOOR)
			display().draw_tile(th.t_pos, FLOOR);
		else if (th.t_oldch == FLOOR && !cansee(th.t_pos.y, th.t_pos.x)
				&& !player.body.t_flags.test(SEEMONST))
			display().draw_tile(th.t_pos, ' ');
		else
			display().draw_tile(th.t_pos, th.t_oldch);
	}
	oroom = th.t_room;
	if (!(ch_ret == th.t_pos))
	{
		if (!(th.t_room = roomin(ch_ret))) {
			th.t_room	= oroom;
			return;
		}
		if (oroom != th.t_room)
			th.t_dest	= find_dest(th);
		th.t_pos = ch_ret;
	}

	if (see_monst(th)) {
		th.t_oldch = display().tile_at(ch_ret);
		display().draw_tile(ch_ret, th.t_disguise,
				level.flags_at(ch_ret).test(MapFlag::Passage) ? TileStyle::Inverse : TileStyle::Normal);
	}
	else if (player.body.t_flags.test(SEEMONST))
	{
		th.t_oldch = display().tile_at(ch_ret);
		display().draw_tile(ch_ret, th.t_type, TileStyle::Inverse);
	}
	else
		th.t_oldch = '@';

	if (th.t_oldch == FLOOR && level.room(*oroom).r_flags.test(RoomFlag::Dark))
		th.t_oldch = ' ';
}

/*
 * see_monst:
 *	Return true if the hero can see the monster
 */
bool
see_monst(const Creature &mp)
{
	rogue::Player &player = game().player;
	if (player.body.t_flags.test(ISBLIND))
		return	false;
	if (mp.t_flags.test(ISINVIS) && !player.body.t_flags.test(CANSEE))
		return	false;
	if (distance_sq(mp.t_pos, player.body.t_pos) >= LAMPDIST &&
	  ((mp.t_room != player.body.t_room || game().level.room(*mp.t_room).r_flags.test(RoomFlag::Dark) ||
	  game().level.room(*mp.t_room).r_flags.test(RoomFlag::Maze))))
		return false;
	/*
	 * If we are seeing	the enemy of a vorpally	enchanted weapon for the first
	 * time, give the player a hint as to what that weapon is good for.
	 */
	if (player.weapon_item() && mp.t_type == player.weapon_item()->o_enemy
	  && !player.weapon_item()->o_flags.test(DIDFLASH))
	{
		player.weapon_item()->o_flags.set(DIDFLASH);
		msg(flashmsg, w_names[player.weapon_item()->which<WeaponType>()], game().options.brief() ? "" : intense);
	}
	return true;
}

/*
 * start_run:
 *	Set a monster running after something or stop it from running
 *	(for	when it	dies)
 */
void
start_run(Coord runner)
{
	Maybe<Creature> tp;

	/*
	 * If we couldn't find him,	something is funny
	 */
	tp = moat(runner.y, runner.x);
	if (tp) {
		/*
		 *	Start the beastie running
		 */
		tp->t_flags.set(ISRUN);
		tp->t_flags.unset(ISHELD);
		tp->t_dest	= find_dest(*tp);
	}
	else if constexpr (rogue::config::debug_checks)
		debug("start_run: moat == null ???");
}

/*
 * chase:
 *	Find	the spot for the chaser(er) to move closer to the
 *	chasee(ee).
 */
static void
chase(Creature &tp, Coord ee)
{
	int	x, y;
	int	dist, thisdist;
	Maybe<Item> obj;
	const Coord er = tp.t_pos;
	unsigned char ch;
	int	plcnt =	1;

	/*
	 * If the thing is confused, let it	move randomly. Phantoms
	 * are slightly confused all of the	time, and bats are
	 * quite confused all the time
	 */
	if ((tp.t_flags.test(ISHUH)	&& rnd(5) != 0)	|| (tp.t_type == 'P' && rnd(5)	== 0)
		|| (tp.t_type	== 'B' && rnd(2) == 0))
	{
		/*
		 * get	a valid	random move
		 */
		ch_ret = rndmove(tp);
		dist =	distance_sq(ch_ret, ee);
		/*
		 * Small chance that it will become un-confused
		 */
		if (rnd(30) ==	17)
			tp.t_flags.unset(ISHUH);
	}
	/*
	 * Otherwise, find the empty spot next to the chaser that is
	 * closest to the chasee.
	 */
	else
	{
		int ey, ex;
		/*
		 * This will eventually hold where we move to get closer
		 * If we can't	find an	empty spot, we stay where we are.
		 */
		dist =	distance_sq(er, ee);
		ch_ret	= er;

		ey = er.y + 1;
		ex = er.x + 1;
		for (x	= er.x	- 1; x <= ex; x++)
		{
			for (y = er.y - 1; y <= ey; y++)
			{
				const Coord tryp = {x, y};

				if (offmap(y,	x) || !diag_ok(er, tryp))
					continue;
				ch = winat(y,	x);
				if (step_ok(ch))
				{
					/*
					 * If it is a scroll, it might be	a scare	monster	scroll
					 * so we need to look it up to see what type it is.
					 */
					if (ch ==	SCROLL)
					{
						for (obj = game().level.objects.first(); obj; obj = game().level.objects.after(*obj))
						{
							if (y ==	obj->o_pos.y &&	x == obj->o_pos.x)
								break;
						}
						if (obj && obj->which<Scroll>() == Scroll::ScareMonster)
							continue;
					}
					/*
					 * If we didn't find any scrolls at this place or	it
					 * wasn't	a scare	scroll,	then this place	counts
					 */
					thisdist = distance_sq(tryp, ee);
					if (thisdist < dist)
					{
						plcnt = 1;
						ch_ret = tryp;
						dist	= thisdist;
					}
					else if (thisdist	== dist	&& rnd(++plcnt)	== 0)
					{
						ch_ret = tryp;
						dist	= thisdist;
					}
				}
			}
		}
	}
}

/*
 * find_dest:
 *	find	the proper destination for the monster
 */
Destination
find_dest(const Creature &tp)
{
	Maybe<Item> obj;
	int prob;
	std::optional<RoomRef> rp;
	rogue::Player &player = game().player;

	if ((prob =	monsters[tp.t_type - 'A'].m_carry) <= 0 || tp.t_room == player.body.t_room
	|| see_monst(tp))
		return Hero{};
	rp = tp.t_room;
	for (obj = game().level.objects.first(); obj; obj = game().level.objects.after(*obj))
	{
	if (obj->o_type == ItemKind::Scroll && obj->which<Scroll>() == Scroll::ScareMonster)
		continue;
	if (roomin(obj->o_pos) == rp && rnd(100) < prob)
	{
		// unless another monster is after it already
		ItemId id = *game().pool.id_of(obj);
		if (std::ranges::none_of(game().level.monsters,
			[id](const Creature &mp) { return mp.t_dest == Destination(id); }))
		return id;
	}
	}
	return Hero{};
}

/*
 * Code for handling the various special properties of the slime
 *
 * slime.c	1.0		(A.I. Design 1.42)	1/17/85
 */

/*
 * Slime_split:
 *	Called when it has been decided that A slime should divide itself
 */

static coord slimy;

static bool	new_slime(Creature &tp);

void
slime_split(Creature &tp)
{
	Maybe<Creature> nslime;

	if (!new_slime(tp) || !(nslime = new_creature()))
		return;
	msg("The slime divides.  Ick!");
	new_monster(*nslime, 'S', slimy);
	if (cansee(slimy.y, slimy.x)) {
		nslime->t_oldch = game().level.at(slimy);
		display().draw_tile(slimy, 'S');
	}
	start_run(slimy);
}

static
bool
new_slime(Creature &tp)
{
	int y, x, ty, tx;
	bool ret;
	Maybe<Creature> ntp;

	ret = false;
	tp.t_flags.set(ISFLY);
	std::optional<Coord> sp = plop_monster((ty = tp.t_pos.y), (tx = tp.t_pos.x));
	if (!sp) {
		/*
		 * There were no open spaces next to this slime, look for other
		 * slimes that might have open spaces next to them.
		 */
		for (y = ty -1; y <= ty+1; y++)
			for (x = tx-1; x <= tx+1; x++)
				if (winat(y, x) == 'S' && (ntp = moat(y, x))) {
					if (ntp->t_flags.test(ISFLY))
						continue;				/* Already done this one */
					if (new_slime(*ntp)) {
						y = ty+2;
						x = tx +2;
					}
				}
	} else {
		ret = true;
		slimy = *sp;
	}
	tp.t_flags.unset(ISFLY);
	return ret;
}

/*
 * Pick an appropriate spot around a central spot for a new monster to spawn
 * (r, c): row, col of central spot
 * Returns the spot for the new monster, or nullopt if no suitable spot
 * around (r, c) is found
 *
 * Original return value was somewhat an abuse of the bool convention,
 * used both as true/false and as an integer for calculating odds.
 * To avoid that, 'inv_odds' was created for the rnd() call
 */
std::optional<Coord>
plop_monster(int r, int c)
{
	int y, x, inv_odds = 0;
	std::optional<Coord> spot;
	unsigned char ch;
	rogue::Player &player = game().player;

	for (y = r-1; y <= r+1; y++)
		for (x = c-1; x <= c+1; x++) {
			/*
			 * Don't put a monster in top of the player.
			 */
			if ((y == player.body.t_pos.y && x == player.body.t_pos.x) || offmap(y,x))
				continue;
			/*
			 * Or anything else nasty
			 */
			if (step_ok(ch = winat(y, x))) {
				if (ch == SCROLL && find_obj(y, x)->which<Scroll>() == Scroll::ScareMonster)
					continue;
				/*
				 * Get first available spot with 100% chance,
				 * then randomly change to next available spot, if any,
				 * with decreasing 1-to-n odds (50%, 33%, 25%, 20%,...)
				 */
				if (rnd(++inv_odds) == 0)
					spot = Coord{x, y};
			}
		}
	return spot;
}

}  // namespace rogue::entities
