#include "rogue.h"

namespace rogue::items::effects {

/*
 * fix_stick:
 *	Set up a new stick
 */
void
fix_stick(Item &cur)
{
	if (game().items.ws_type[cur.which<Stick>()] == "staff")
		cur.o_damage = "2d3";
	else
		cur.o_damage = "1d1";
	cur.o_hurldmg = "1d1";

	cur.charges() = 3 + rnd(5);
	switch (cur.which<Stick>())
	{
	case Stick::Striking:
		cur.o_hplus = 100;
		cur.o_dplus = 3;
		cur.o_damage = "1d8";
		break;
	case Stick::Light:
		cur.charges() = 10 + rnd(10);
		break;
	default:
		break;
	}
}

/*
 * do_zap:
 *	Perform a zap with a wand
 */
void
do_zap()
{
	Item *obj;
	Creature *tp;
	int y, x;
	std::string_view name;
	Stick which_one;
	rogue::Turn &turn = game().turn;
	rogue::Player &player = game().player;

	if ((obj = get_item("zap with", ItemKind::Stick)) == nullptr)
		return;
	which_one = obj->which<Stick>();
	if (obj->o_type != ItemKind::Stick)
	{
		if (obj->o_enemy && obj->charges())
			which_one = Stick::Vorpal;
		else
		{
			msg("you can't zap with that!");
			turn.after = false;
			return;
		}
	}
	if (obj->charges() == 0)
	{
		msg("nothing happens");
		return;
	}
	switch (which_one)
	{
	case Stick::Light:
		/*
		 * Reddy Kilowat wand.  Light up the room
		 */
		if (player.body.t_flags.test(ISBLIND))
			msg("you feel a warm glow around you");
		else
		{
			game().items.ws_know[Stick::Light] = true;
			if (game().level.room(*player.body.t_room).r_flags.test(RoomFlag::Gone))
				msg("the corridor glows and then fades");
			else
				msg("the room is lit by a shimmering blue light");
		}
		if (!game().level.room(*player.body.t_room).r_flags.test(RoomFlag::Gone))
		{
			game().level.room(*player.body.t_room).r_flags.unset(RoomFlag::Dark);
			/*
			 * Light the room and put the player back up
			 */
			enter_room(player.body.t_pos);
		}
		break;
	case Stick::DrainLife:
		/*
		 * Take away 1/2 of hero's hit points, then take it away
		 * evenly from the monsters in the room (or next to hero
		 * if he is in a passage)
		 */
		if (player.body.t_stats.s_hpt < 2)
		{
			msg("you are too weak to use it");
			return;
		}
		else
			drain();
		break;
	case Stick::Polymorph:
	case Stick::TeleportAway:
	case Stick::TeleportTo:
	case Stick::Cancellation:
	case Stick::Vorpal:			/* Special case for vorpal weapon */
	{
		unsigned char monster, oldch;
		int rm;
		coord new_yx;

		y = player.body.t_pos.y;
		x = player.body.t_pos.x;
		while (step_ok(winat(y, x)))
		{
			y += turn.delta.y;
			x += turn.delta.x;
		}
		if ((tp = moat(y, x)) != nullptr)
		{
			unsigned char omonst;

			omonst = monster = tp->t_type;
			if (monster == 'F')
				player.body.t_flags.unset(ISHELD);
			if (which_one == Stick::Vorpal)
			{
				if (monster == obj->o_enemy)
				{
					msg("the {} vanishes in a puff of smoke",
						monsters[monster-'A'].m_name);
					killed(*tp, false);
				}
				else
					msg("you hear a maniacal chuckle in the distance.");
			}
			else if (which_one == Stick::Polymorph)
			{
				List<Item> pp;

				pp = std::move(tp->t_pack);
				game().level.monsters.remove(tp);
				if (see_monst(*tp))
					display().draw_tile({x, y}, game().level.at(y, x));
				oldch = tp->t_oldch;
				turn.delta.y = y;
				turn.delta.x = x;
				new_monster(*tp, monster = rnd(26) + 'A', turn.delta);
				if (see_monst(*tp))
					display().draw_tile({x, y}, monster);
				tp->t_oldch = oldch;
				tp->t_pack = std::move(pp);
				game().items.ws_know[Stick::Polymorph] |= (monster != omonst);
			}
			else if (which_one == Stick::Cancellation)
			{
				tp->t_flags.set(ISCANC);
				tp->t_flags.unset(ISINVIS|CANHUH);
				tp->t_disguise = tp->t_type;
			}
			else
			{
				if (see_monst(*tp))
					display().draw_tile({x, y}, tp->t_oldch);
				if (which_one == Stick::TeleportAway)
				{
					tp->t_oldch = '@';
					do
					{
						rm = rnd_room();
						new_yx = rnd_pos(game().level.rooms[rm]);
					}  while (!(is_floor(winat(new_yx.y, new_yx.x))));
					tp->t_pos = new_yx;
					if (see_monst(*tp))
						display().draw_tile(tp->t_pos, tp->t_disguise);
					else if (player.body.t_flags.test(SEEMONST))
						display().draw_tile(tp->t_pos, tp->t_disguise, TileStyle::Inverse);
				}
				else /* it MUST BE at Stick::TeleportTo */
				{
					tp->t_pos.y = player.body.t_pos.y + turn.delta.y;
					tp->t_pos.x = player.body.t_pos.x + turn.delta.x;
				}
				if (tp->t_type == 'F')
					player.body.t_flags.unset(ISHELD);
				if (tp->t_pos.y != y || tp->t_pos.x != x)
					tp->t_oldch = display().tile_at(tp->t_pos);
			}
			tp->t_dest = Hero{};
			tp->t_flags.set(ISRUN);
		}
	}
		break;
	case Stick::MagicMissile:
	{
		Item bolt;

		game().items.ws_know[Stick::MagicMissile] = true;
		bolt.o_type = ItemKind::Missile;
		bolt.o_hurldmg = "1d8";
		bolt.o_hplus = 1000;
		bolt.o_dplus = 1;
		bolt.o_flags = ISMISL;
		if (player.weapon_item() != nullptr)
			bolt.o_launch = launched_by(player.weapon_item()->which<WeaponType>());
		do_motion(bolt, turn.delta.y, turn.delta.x);
		if ((tp = moat(bolt.o_pos.y, bolt.o_pos.x)) != nullptr && !save_throw(SaveThrow::Magic, *tp))
			hit_monster(bolt.o_pos.y, bolt.o_pos.x, bolt);
		else
		msg("the missle vanishes with a puff of smoke");
	}
		break;
	case Stick::Striking:
		turn.delta.y += player.body.t_pos.y;
		turn.delta.x += player.body.t_pos.x;
		if ((tp = moat(turn.delta.y, turn.delta.x)) != nullptr)
		{
			if (rnd(20) == 0)
			{
				obj->o_damage = "3d8";
				obj->o_dplus = 9;
			}
			else
			{
				obj->o_damage = "2d8";
				obj->o_dplus = 4;
			}
			fight(turn.delta, tp->t_type, *obj, false);
		}
		break;
	case Stick::HasteMonster:
	case Stick::SlowMonster:
		y = player.body.t_pos.y;
		x = player.body.t_pos.x;
		while (step_ok(winat(y, x)))
		{
			y += turn.delta.y;
			x += turn.delta.x;
		}
		if ((tp = moat(y, x)) != nullptr)
		{
			if (which_one == Stick::HasteMonster)
			{
				if (tp->t_flags.test(ISSLOW))
					tp->t_flags.unset(ISSLOW);
				else
					tp->t_flags.set(ISHASTE);
			}
			else
			{
				if (tp->t_flags.test(ISHASTE))
					tp->t_flags.unset(ISHASTE);
				else
					tp->t_flags.set(ISSLOW);
				tp->t_turn = true;
			}
			turn.delta.y = y;
			turn.delta.x = x;
			start_run(turn.delta);
		}
		break;
	case Stick::Lightning:
	case Stick::Fire:
	case Stick::Cold:
		if (which_one == Stick::Lightning)
			name = "bolt";
		else if (which_one == Stick::Fire)
			name = "flame";
		else
			name = "ice";
		fire_bolt(player.body.t_pos, turn.delta, name);
		game().items.ws_know[which_one] = true;
		break;
	default:
		if constexpr (rogue::config::debug_checks)
			debug("what a bizarre schtick!");
		break;
	}
	if (--obj->charges() < 0)
		obj->charges() = 0;
}

/*
 * drain:
 *	Do drain hit points from player schtick
 */
void
drain()
{
	Creature *mp;
	int cnt;
	std::optional<RoomRef> corp;
	Creature **dp;
	bool inpass;
	Creature *drainee[40];
	rogue::Player &player = game().player;
	rogue::Level &level = game().level;

	/*
	 * First cnt how many things we need to spread the hit points among
	 */
	cnt = 0;
	if (level.at(player.body.t_pos) == DOOR)
		corp = level.passage_at(player.body.t_pos);
	else
		corp = std::nullopt;
	inpass = level.room(*player.body.t_room).r_flags.test(RoomFlag::Gone);
	dp = drainee;
	for (mp = level.monsters.first(); mp != nullptr; mp = level.monsters.after(mp))
		if (mp->t_room == player.body.t_room || mp->t_room == corp ||
			(inpass && level.at(mp->t_pos) == DOOR &&
			level.passage_at(mp->t_pos) == player.body.t_room))
			*dp++ = mp;
	if ((cnt = dp - drainee) == 0)
	{
		msg("you have a tingling feeling");
		return;
	}
	*dp = nullptr;
	player.body.t_stats.s_hpt /= 2;
	cnt = player.body.t_stats.s_hpt / cnt + 1;
	/*
	 * Now zot all of the monsters
	 */
	for (dp = drainee; *dp; dp++)
	{
		mp = *dp;
		if ((mp->t_stats.s_hpt -= cnt) <= 0)
			killed(*mp, see_monst(*mp));
		else
			start_run(mp->t_pos);
	}
}

/*
 * fire_bolt:
 *	Fire a bolt in a given direction from a specific starting place. The
 *	rogue fired it if it starts where he stands (no monster stands there).
 *	dir is reversed each time the bolt bounces, as `a` reuses it.
 */
void
fire_bolt(Coord start, Coord &dir, std::string_view name)
{
	unsigned char dirch = 0, ch;
	Creature *tp;
	bool hit_hero, used, changed;
	int i, j;
	coord pos;
	rogue::Player &player = game().player;
	struct {
		coord s_pos;
		unsigned char s_under;
	} spotpos[BOLT_LENGTH*2];
	Item bolt;
	bool is_frost;

	is_frost = (name == "frost");
	bolt.o_type = ItemKind::Weapon;
	bolt.set_which(WeaponType::Flame);
	bolt.o_damage = bolt.o_hurldmg = "6d6";
	bolt.o_hplus = 30;
	bolt.o_dplus = 0;
	w_names[WeaponType::Flame] = name;
	switch (dir.y + dir.x) {
		case 0: dirch = '/'; break;
		case 1: case -1: dirch = (dir.y == 0 ? '-' : '|'); break;
		case 2: case -2: dirch = '\\';
		break;
	}
	const bool by_hero = (start == player.body.t_pos);
	pos = start;
	hit_hero = !by_hero;
	used = false;
	changed = false;
	for (i = 0; i < BOLT_LENGTH && !used; i++) {
		pos.y += dir.y;
		pos.x += dir.x;
		ch = winat(pos.y, pos.x);
		spotpos[i].s_pos = pos;
		if ((spotpos[i].s_under = display().tile_at(pos)) == dirch)
			spotpos[i].s_under = 0;
		switch (ch) {
		case DOOR:
		case HWALL:
		case VWALL:
		case ULWALL:
		case URWALL:
		case LLWALL:
		case LRWALL:
		case ' ':
			if (!changed)
				hit_hero = !hit_hero;
			changed = false;
			dir.y = -dir.y;
			dir.x = -dir.x;
			i--;
			msg("the {} bounces", name);
			break;
		default:
			if (!hit_hero && (tp = moat(pos.y, pos.x)) != nullptr) {
				hit_hero = true;
				changed = !changed;
				if (tp->t_oldch != '@')
					tp->t_oldch = game().level.at(pos);
				if (!save_throw(SaveThrow::Magic, *tp) || is_frost) {
					bolt.o_pos = pos;
					used = true;
					if (tp->t_type == 'D' && name == "flame")
						msg("the flame bounces off the dragon");
					else {
						hit_monster(pos.y, pos.x, bolt);
						if (display().tile_at(pos) != dirch)
							spotpos[i].s_under = display().tile_at(pos);
					}
				} else if (ch != 'X' || tp->t_disguise == 'X') {
					if (by_hero)
						start_run(pos);
					msg("the {} whizzes past the {}",
						name, monsters[ch-'A'].m_name);
				}
			} else if (hit_hero && (pos == player.body.t_pos)) {
				hit_hero = false;
				changed = !changed;
				if (!save(SaveThrow::Magic)) {
					if (is_frost) {
						msg("You are frozen by a blast of frost{}.",
							noterse(" from the Ice Monster"));
						if (player.no_command < 20)
							player.no_command += spread(7);
					} else if ((player.body.t_stats.s_hpt -= roll(6, 6)) <= 0) {
						if (by_hero)
							death('b');
						else
							death(moat(start.y, start.x)->t_type);
					}
					used = true;
					if (!is_frost)
						msg("you are hit by the {}", name);
				} else
					msg("the {} whizzes by you", name);
			}
			tick_pause();
			display().draw_tile(pos, dirch, is_frost ? TileStyle::FrostBolt : TileStyle::Bolt);
			break;
		}
	}
	for (j = 0; j < i; j++) {
		tick_pause();
		if (spotpos[j].s_under)
			display().draw_tile(spotpos[j].s_pos, spotpos[j].s_under);
	}
}

/*
 * charge_str:
 *	Return an appropriate string for a wand charge
 */
std::string
charge_str(const Item &obj)
{
	if (!obj.o_flags.test(ISKNOW))
		return "";
	return std::format(" [{} charges]", obj.charges());
}

}  // namespace rogue::items::effects
