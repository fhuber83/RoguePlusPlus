/*
 * Code	for one	creature to chase another
 *
 * chase.c	1.32	(A.I. Design) 12/12/84
 */

#include "entities/MonsterAI.hpp"

#include <algorithm>
#include <cstdlib>
#include <optional>

#include "core/Config.hpp"
#include "core/Coord.hpp"
#include "core/Glyphs.hpp"
#include "core/Math.hpp"
#include "core/Maybe.hpp"
#include "entities/Creature.hpp"
#include "entities/Item.hpp"
#include "entities/MonsterCatalog.hpp"
#include "game/Game.hpp"
#include "game/Id.hpp"
#include "game/Messages.hpp"
#include "game/Movement.hpp"
#include "game/Pool.hpp"
#include "items/ItemCatalog.hpp"
#include "items/Kinds.hpp"
#include "items/effects/Wand.hpp"
#include "items/effects/Weapon.hpp"
#include "rules/Combat.hpp"
#include "ui/Display.hpp"
#include "world/Level.hpp"
#include "world/MapFlags.hpp"
#include "world/Room.hpp"
#include "world/RoomRef.hpp"
#include "world/Rooms.hpp"

namespace rogue::entities {

namespace {

void	do_chase(Creature &th);
Coord	chase(Creature &tp, Coord ee);

}  // namespace

constexpr int DRAGONSHOT = 5;	/* one chance in DRAGONSHOT that a dragon will flame */

/*
 * runners:
 *	Make all the running monsters move.
 */
void
runners()
{
	rogue::Player &player = game().player;

	for (Maybe<Creature> tp = game().level.monsters.first(); tp; tp = game().level.monsters.after(*tp)) {
		if (!tp->t_flags.test(CreatureFlag::Held) && tp->t_flags.test(CreatureFlag::Running)) {
			const CreatureId id = *game().pool.id_of(*tp);
			int dist = distance_sq(player.body.t_pos, tp->t_pos);
			if	(!(tp->t_flags.test(CreatureFlag::Slow) || (tp->t_type == 'S' && dist > 3)) || tp->t_turn)
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
			if (tp->t_flags.test(CreatureFlag::Hasted))
				do_chase(*tp);
			if (!game().level.monsters.contains(id))
				break;
			dist = distance_sq(player.body.t_pos, tp->t_pos);
			if (tp->t_flags.test(CreatureFlag::Flying) && dist > 3)
				do_chase(*tp);
			if (!game().level.monsters.contains(id))
				break;
			tp->t_turn ^= true;
		}
	}
}

namespace {

/*
 * do_chase:
 *	Make one thing chase another.
 */
void
do_chase(Creature &th)
{
	rogue::Player &player = game().player;
	world::Level &level = game().level;

	std::optional<RoomRef> rer = th.t_room;		/* Find room of chaser */
	if (th.t_flags.test(CreatureFlag::Greedy) && level.room(*rer).r_goldval == 0)
		th.t_dest = Hero{};	/*	If gold	has been taken,	run after hero */
	std::optional<RoomRef> ree = player.body.t_room;	/* room of chasee */
	if (th.t_dest != Destination(Hero{}))	/*	Find room of chasee */
		ree = world::roomin(game().where(*th.t_dest));
	if (!ree)
		return;
	/*
	 * We don't	count doors as inside rooms for	this routine
	 */
	bool door = (level.at(th.t_pos) == DOOR);

	/*
	 * If the object of	our desire is in a different room,
	 * and we are not in a maze, run to	the door nearest to
	 * our goal.
	 */
	int mindist = 32767;
	Coord target;				/* Temporary	destination for	chaser */
	for (;;) {
		if (rer != ree && !level.room(*rer).r_flags.test(RoomFlag::Maze))
		{
			const world::Room &from = level.room(*rer);
			const Coord dest = game().where(*th.t_dest);

			for (int i = 0; i < from.r_nexits; i++) {	/*	loop through doors */
				int dist = distance_sq(dest, from.r_exit[i]);
				if	(dist <	mindist) {
					target = from.r_exit[i];
					mindist = dist;
				}
			}
			if (door) {
				rer = level.passage_at(th.t_pos);
				door = false;
				continue;
			}
		} else {
			target =	game().where(*th.t_dest);
			/*
			 * For	monsters which can fire	bolts at the poor hero,	we check to
			 * see	if (a) the hero	in on a	straight line from it, and (b) that
			 * it is within shooting distance, but	outside	of striking range.
			 */
			int dist;
			if ((th.t_type == 'D' || th.t_type == 'I')
				&&	(th.t_pos.y ==	player.body.t_pos.y || th.t_pos.x == player.body.t_pos.x
				 || abs(th.t_pos.y - player.body.t_pos.y) == abs(th.t_pos.x - player.body.t_pos.x))
				&&	((dist=distance_sq(th.t_pos, player.body.t_pos)) > 2
				 && dist <= items::effects::BOLT_LENGTH	* items::effects::BOLT_LENGTH)
				&&	!th.t_flags.test(CreatureFlag::Cancelled) && rnd(DRAGONSHOT) == 0)
			{
				game().turn.running = false;
				game().turn.delta.y = sign(player.body.t_pos.y - th.t_pos.y);
				game().turn.delta.x = sign(player.body.t_pos.x - th.t_pos.x);
				items::effects::fire_bolt(th.t_pos, game().turn.delta, th.t_type == 'D' ? "flame" : "frost");
				return;
			}
		}
		break;
	}
	/*
	 * This now	contains what we want to run to	this time
	 * so we run to it.	 If we hit it we either	want to	fight it
	 * or stop running
	 */
	const Coord ch_ret = chase(th, target);	/* Where chasing takes	you */
	if (ch_ret == player.body.t_pos) {
		rules::attack(th);
		return;
	} else if (ch_ret == game().where(*th.t_dest)) {
		// A walk by first()/after(): the body takes obj out of the list
		for (Maybe<Item> obj = level.objects.first(); obj; obj = level.objects.after(*obj))
			if	(th.t_dest == Destination(*game().pool.id_of(obj))) {
				level.objects.remove(*obj);
				th.t_pack.push_front(*obj);
				unsigned char oldchar = level.at(obj->o_pos) =
				level.room(*th.t_room).r_flags.test(RoomFlag::Gone) ? PASSAGE : FLOOR;
				if (world::cansee(obj->o_pos.y, obj->o_pos.x))
					ui::display().draw_tile(obj->o_pos, oldchar);
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
		if	(th.t_oldch ==	' ' && world::cansee(th.t_pos.y, th.t_pos.x)
			   && level.map[world::Level::index(th.t_pos)] == FLOOR)
			ui::display().draw_tile(th.t_pos, FLOOR);
		else if (th.t_oldch == FLOOR && !world::cansee(th.t_pos.y, th.t_pos.x)
				&& !player.body.t_flags.test(CreatureFlag::SeeMonst))
			ui::display().draw_tile(th.t_pos, ' ');
		else
			ui::display().draw_tile(th.t_pos, th.t_oldch);
	}
	std::optional<RoomRef> oroom = th.t_room;
	if (!(ch_ret == th.t_pos))
	{
		if (!(th.t_room = world::roomin(ch_ret))) {
			th.t_room	= oroom;
			return;
		}
		if (oroom != th.t_room)
			th.t_dest	= find_dest(th);
		th.t_pos = ch_ret;
	}

	if (see_monst(th)) {
		th.t_oldch = ui::display().tile_at(ch_ret);
		ui::display().draw_tile(ch_ret, th.t_disguise,
				level.flags_at(ch_ret).test(MapFlag::Passage) ? ui::TileStyle::Inverse : ui::TileStyle::Normal);
	}
	else if (player.body.t_flags.test(CreatureFlag::SeeMonst))
	{
		th.t_oldch = ui::display().tile_at(ch_ret);
		ui::display().draw_tile(ch_ret, th.t_type, ui::TileStyle::Inverse);
	}
	else
		th.t_oldch = '@';

	if (th.t_oldch == FLOOR && level.room(*oroom).r_flags.test(RoomFlag::Dark))
		th.t_oldch = ' ';
}

}  // namespace

/*
 * see_monst:
 *	Return true if the hero can see the monster
 */
bool
see_monst(const Creature &mp)
{
	rogue::Player &player = game().player;
	if (player.body.t_flags.test(CreatureFlag::Blind))
		return	false;
	if (mp.t_flags.test(CreatureFlag::Invisible) && !player.body.t_flags.test(CreatureFlag::SeeInvisible))
		return	false;
	if (distance_sq(mp.t_pos, player.body.t_pos) >= world::LAMPDIST &&
	  ((mp.t_room != player.body.t_room || game().level.room(*mp.t_room).r_flags.test(RoomFlag::Dark) ||
	  game().level.room(*mp.t_room).r_flags.test(RoomFlag::Maze))))
		return false;
	/*
	 * If we are seeing	the enemy of a vorpally	enchanted weapon for the first
	 * time, give the player a hint as to what that weapon is good for.
	 */
	if (player.weapon_item() && mp.t_type == player.weapon_item()->o_enemy
	  && !player.weapon_item()->o_flags.test(ItemFlag::DidFlash))
	{
		player.weapon_item()->o_flags.set(ItemFlag::DidFlash);
		msg(items::effects::flashmsg, items::w_names[player.weapon_item()->which<WeaponType>()], game().options.brief() ? "" : items::effects::intense);
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
	/*
	 * If we couldn't find him,	something is funny
	 */
	Maybe<Creature> tp = game().level.monster_at(runner);
	if (tp) {
		/*
		 *	Start the beastie running
		 */
		tp->t_flags.set(CreatureFlag::Running);
		tp->t_flags.unset(CreatureFlag::Held);
		tp->t_dest	= find_dest(*tp);
	}
	else if constexpr (rogue::config::debug_checks)
		debug("start_run: moat == null ???");
}

namespace {

/*
 * chase:
 *	Find	the spot for the chaser(er) to move closer to the
 *	chasee(ee), and return it.
 */
Coord
chase(Creature &tp, Coord ee)
{
	const Coord er = tp.t_pos;
	Coord ch_ret;
	int	dist;
	int	plcnt =	1;

	/*
	 * If the thing is confused, let it	move randomly. Phantoms
	 * are slightly confused all of the	time, and bats are
	 * quite confused all the time
	 */
	if ((tp.t_flags.test(CreatureFlag::Confused)	&& rnd(5) != 0)	|| (tp.t_type == 'P' && rnd(5)	== 0)
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
			tp.t_flags.unset(CreatureFlag::Confused);
	}
	/*
	 * Otherwise, find the empty spot next to the chaser that is
	 * closest to the chasee.
	 */
	else
	{
		/*
		 * This will eventually hold where we move to get closer
		 * If we can't	find an	empty spot, we stay where we are.
		 */
		dist =	distance_sq(er, ee);
		ch_ret	= er;

		int ey = er.y + 1;
		int ex = er.x + 1;
		for (int x = er.x - 1; x <= ex; x++)
		{
			for (int y = er.y - 1; y <= ey; y++)
			{
				const Coord tryp = {x, y};

				if (world::Level::off_map({x, y}) || !game().level.diagonal_ok(er, tryp))
					continue;
				unsigned char ch = game().level.seen_at({x, y});
				if (step_ok(ch))
				{
					/*
					 * If it is a scroll, it might be	a scare	monster	scroll
					 * so we need to look it up to see what type it is.
					 */
					if (ch ==	SCROLL)
					{
						Maybe<Item> obj = game().level.object_at({x, y});
						if (obj && obj->which<Scroll>() == Scroll::ScareMonster)
							continue;
					}
					/*
					 * If we didn't find any scrolls at this place or	it
					 * wasn't	a scare	scroll,	then this place	counts
					 */
					int thisdist = distance_sq(tryp, ee);
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
	return ch_ret;
}

}  // namespace

/*
 * find_dest:
 *	find	the proper destination for the monster
 */
Destination
find_dest(const Creature &tp)
{
	rogue::Player &player = game().player;

	int prob = monsters[tp.t_type - 'A'].m_carry;
	if (prob <= 0 || tp.t_room == player.body.t_room
	|| see_monst(tp))
		return Hero{};
	std::optional<RoomRef> rp = tp.t_room;
	for (Item &obj : game().level.objects)
	{
	if (obj.o_type == ItemKind::Scroll && obj.which<Scroll>() == Scroll::ScareMonster)
		continue;
	if (world::roomin(obj.o_pos) == rp && rnd(100) < prob)
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

namespace {

std::optional<Coord>	new_slime(Creature &tp);

}  // namespace

void
slime_split(Creature &tp)
{
	std::optional<Coord> slime_at = new_slime(tp);
	if (!slime_at)
		return;
	const Coord slimy = *slime_at;
	Maybe<Creature> nslime = new_creature();
	if (!nslime)
		return;
	msg("The slime divides.  Ick!");
	new_monster(*nslime, 'S', slimy);
	if (world::cansee(slimy.y, slimy.x)) {
		nslime->t_oldch = game().level.at(slimy);
		ui::display().draw_tile(slimy, 'S');
	}
	start_run(slimy);
}

namespace {

std::optional<Coord>
new_slime(Creature &tp)
{
	std::optional<Coord> ret;
	tp.t_flags.set(CreatureFlag::Flying);
	int ty = tp.t_pos.y;
	int tx = tp.t_pos.x;
	std::optional<Coord> sp = plop_monster(ty, tx);
	if (!sp) {
		/*
		 * There were no open spaces next to this slime, look for other
		 * slimes that might have open spaces next to them.
		 */
		for (int y = ty -1; y <= ty+1; y++)
			for (int x = tx-1; x <= tx+1; x++) {
				if (game().level.seen_at({x, y}) != 'S')
					continue;
				Maybe<Creature> ntp = game().level.monster_at({x, y});
				if (!ntp || ntp->t_flags.test(CreatureFlag::Flying))
					continue;				/* none, or already done this one */
				// One that divides there doesn't make this one divide
				if (new_slime(*ntp)) {
					y = ty+2;
					x = tx +2;
				}
			}
	} else
		ret = sp;
	tp.t_flags.unset(CreatureFlag::Flying);
	return ret;
}

}  // namespace

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
	int inv_odds = 0;
	std::optional<Coord> spot;
	rogue::Player &player = game().player;

	for (int y = r-1; y <= r+1; y++)
		for (int x = c-1; x <= c+1; x++) {
			/*
			 * Don't put a monster in top of the player.
			 */
			if ((y == player.body.t_pos.y && x == player.body.t_pos.x) || world::Level::off_map({x, y}))
				continue;
			/*
			 * Or anything else nasty
			 */
			unsigned char ch = game().level.seen_at({x, y});
			if (step_ok(ch)) {
				if (ch == SCROLL && game().level.object_at({x, y})->which<Scroll>() == Scroll::ScareMonster)
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

/*
 * aggravate:
 *	Aggravate all the monsters on this level
 */
void
aggravate()
{
	for (Creature &mi : game().level.monsters)
		start_run(mi.t_pos);
}

}  // namespace rogue::entities
