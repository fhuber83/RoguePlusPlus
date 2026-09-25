#include "rogue.h"

namespace rogue::items::effects {

/*
 * fix_stick:
 *	Set up a new stick
 */
void
fix_stick(Item *cur)
{
	if (strcmp(game().items.ws_type[cur->o_which], "staff") == 0)
		cur->o_damage = "2d3";
	else
		cur->o_damage = "1d1";
	cur->o_hurldmg = "1d1";

	cur->o_charges = 3 + rnd(5);
	switch (cur->o_which)
	{
	case WS_HIT:
		cur->o_hplus = 100;
		cur->o_dplus = 3;
		cur->o_damage = "1d8";
		break;
	case WS_LIGHT:
		cur->o_charges = 10 + rnd(10);
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
	const char *name;
	int which_one;
	rogue::Turn &turn = game().turn;
	rogue::Player &player = game().player;

	if ((obj = get_item("zap with", ItemKind::Stick)) == NULL)
		return;
	which_one = obj->o_which;
	if (obj->o_type != ItemKind::Stick)
	{
		if (obj->o_enemy && obj->o_charges)
			which_one = MAXSTICKS;
		else
		{
			msg("you can't zap with that!");
			turn.after = FALSE;
			return;
		}
	}
	if (obj->o_charges == 0)
	{
		msg("nothing happens");
		return;
	}
	switch (which_one)
	{
	case WS_LIGHT:
		/*
		 * Reddy Kilowat wand.  Light up the room
		 */
		if (player.body.t_flags.test(ISBLIND))
			msg("you feel a warm glow around you");
		else
		{
			game().items.ws_know[WS_LIGHT] = TRUE;
			if (proom->r_flags.test(RoomFlag::Gone))
				msg("the corridor glows and then fades");
			else
				msg("the room is lit by a shimmering blue light");
		}
		if (!proom->r_flags.test(RoomFlag::Gone))
		{
			proom->r_flags.unset(RoomFlag::Dark);
			/*
			 * Light the room and put the player back up
			 */
			enter_room(&hero);
		}
		break;
	case WS_DRAIN:
		/*
		 * Take away 1/2 of hero's hit points, then take it away
		 * evenly from the monsters in the room (or next to hero
		 * if he is in a passage)
		 */
		if (pstats.s_hpt < 2)
		{
			msg("you are too weak to use it");
			return;
		}
		else
			drain();
		break;
	case WS_POLYMORPH:
	case WS_TELAWAY:
	case WS_TELTO:
	case WS_CANCEL:
	case MAXSTICKS:			/* Special case for vorpal weapon */
	{
		unsigned char monster, oldch;
		int rm;
		coord new_yx;

		y = hero.y;
		x = hero.x;
		while (step_ok(winat(y, x)))
		{
			y += turn.delta.y;
			x += turn.delta.x;
		}
		if ((tp = moat(y, x)) != NULL)
		{
			unsigned char omonst;

			omonst = monster = tp->t_type;
			if (monster == 'F')
				player.body.t_flags.unset(ISHELD);
			if (which_one == MAXSTICKS)
			{
				if (monster == obj->o_enemy)
				{
					msg("the %s vanishes in a puff of smoke",
						monsters[monster-'A'].m_name);
					killed(tp, FALSE);
				}
				else
					msg("you hear a maniacal chuckle in the distance.");
			}
			else if (which_one == WS_POLYMORPH)
			{
				List<Item> pp;

				pp = std::move(tp->t_pack);
				detach(game().level.monsters, tp);
				if (see_monst(tp))
					display().draw_tile({x, y}, chat(y, x));
				oldch = tp->t_oldch;
				turn.delta.y = y;
				turn.delta.x = x;
				new_monster(tp, monster = rnd(26) + 'A', &turn.delta);
				if (see_monst(tp))
					display().draw_tile({x, y}, monster);
				tp->t_oldch = oldch;
				tp->t_pack = std::move(pp);
				game().items.ws_know[WS_POLYMORPH] |= (monster != omonst);
			}
			else if (which_one == WS_CANCEL)
			{
				tp->t_flags.set(ISCANC);
				tp->t_flags.unset(ISINVIS|CANHUH);
				tp->t_disguise = tp->t_type;
			}
			else
			{
				if (see_monst(tp))
					display().draw_tile({x, y}, tp->t_oldch);
				if (which_one == WS_TELAWAY)
				{
					tp->t_oldch = '@';
					do
					{
						rm = rnd_room();
						new_yx = tp->t_pos;
						rnd_pos(&game().level.rooms[rm], &new_yx);
					}  while (!(isfloor(winat(new_yx.y, new_yx.x))));
					tp->t_pos = new_yx;
					if (see_monst(tp))
						display().draw_tile(tp->t_pos, tp->t_disguise);
					else if (player.body.t_flags.test(SEEMONST))
						display().draw_tile(tp->t_pos, tp->t_disguise, TileStyle::Inverse);
				}
				else /* it MUST BE at WS_TELTO */
				{
					tp->t_pos.y = hero.y + turn.delta.y;
					tp->t_pos.x = hero.x + turn.delta.x;
				}
				if (tp->t_type == 'F')
					player.body.t_flags.unset(ISHELD);
				if (tp->t_pos.y != y || tp->t_pos.x != x)
					tp->t_oldch = display().tile_at(tp->t_pos);
			}
			tp->t_dest = &hero;
			tp->t_flags.set(ISRUN);
		}
	}
		break;
	case WS_MISSILE:
	{
		Item bolt;

		game().items.ws_know[WS_MISSILE] = TRUE;
		bolt.o_type = ItemKind::Missile;
		bolt.o_hurldmg = "1d8";
		bolt.o_hplus = 1000;
		bolt.o_dplus = 1;
		bolt.o_flags = ISMISL;
		if (player.weapon != NULL)
			bolt.o_launch = player.weapon->o_which;
		do_motion(&bolt, turn.delta.y, turn.delta.x);
		if ((tp = moat(bolt.o_pos.y, bolt.o_pos.x)) != NULL && !save_throw(VS_MAGIC, tp))
			hit_monster(unc(bolt.o_pos), &bolt);
		else
		msg("the missle vanishes with a puff of smoke");
	}
		break;
	case WS_HIT:
		turn.delta.y += hero.y;
		turn.delta.x += hero.x;
		if ((tp = moat(turn.delta.y, turn.delta.x)) != NULL)
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
			fight(&turn.delta, tp->t_type, obj, FALSE);
		}
		break;
	case WS_HASTE_M:
	case WS_SLOW_M:
		y = hero.y;
		x = hero.x;
		while (step_ok(winat(y, x)))
		{
			y += turn.delta.y;
			x += turn.delta.x;
		}
		if ((tp = moat(y, x)) != NULL)
		{
			if (which_one == WS_HASTE_M)
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
				tp->t_turn = TRUE;
			}
			turn.delta.y = y;
			turn.delta.x = x;
			start_run(&turn.delta);
		}
		break;
	case WS_ELECT:
	case WS_FIRE:
	case WS_COLD:
		if (which_one == WS_ELECT)
			name = "bolt";
		else if (which_one == WS_FIRE)
			name = "flame";
		else
			name = "ice";
		fire_bolt(&hero, &turn.delta, name);
		game().items.ws_know[which_one] = TRUE;
		break;
#ifdef DEBUG
	default:
		msg("what a bizarre schtick!");
		break;
#endif
	}
	if (--obj->o_charges < 0)
		obj->o_charges = 0;
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
	struct room *corp;
	Creature **dp;
	bool inpass;
	Creature *drainee[40];

	/*
	 * First cnt how many things we need to spread the hit points among
	 */
	cnt = 0;
	if (chat(hero.y, hero.x) == DOOR)
		corp = &game().level.passages[flat(hero.y, hero.x) & F_PNUM];
	else
		corp = NULL;
	inpass = proom->r_flags.test(RoomFlag::Gone);
	dp = drainee;
	for (mp = game().level.monsters.first(); mp != NULL; mp = game().level.monsters.after(mp))
		if (mp->t_room == proom || mp->t_room == corp ||
			(inpass && chat(mp->t_pos.y, mp->t_pos.x) == DOOR &&
			&game().level.passages[flat(mp->t_pos.y, mp->t_pos.x) & F_PNUM] == proom))
			*dp++ = mp;
	if ((cnt = dp - drainee) == 0)
	{
		msg("you have a tingling feeling");
		return;
	}
	*dp = NULL;
	pstats.s_hpt /= 2;
	cnt = pstats.s_hpt / cnt + 1;
	/*
	 * Now zot all of the monsters
	 */
	for (dp = drainee; *dp; dp++)
	{
		mp = *dp;
		if ((mp->t_stats.s_hpt -= cnt) <= 0)
			killed(mp, see_monst(mp));
		else
			start_run(&mp->t_pos);
	}
}

/*
 * fire_bolt:
 *	Fire a bolt in a given direction from a specific starting place
 */
void
fire_bolt(coord *start, coord *dir, const char *name)
{
	unsigned char dirch = 0, ch;
	Creature *tp;
	bool hit_hero, used, changed;
	int i, j;
	coord pos;
	struct {
		coord s_pos;
		unsigned char s_under;
	} spotpos[BOLT_LENGTH*2];
	Item bolt;
	bool is_frost;

	is_frost = (strcmp(name, "frost") == 0);
	bolt.o_type = ItemKind::Weapon;
	bolt.o_which = FLAME;
	bolt.o_damage = bolt.o_hurldmg = "6d6";
	bolt.o_hplus = 30;
	bolt.o_dplus = 0;
	w_names[FLAME] = name;
	switch (dir->y + dir->x) {
		case 0: dirch = '/'; break;
		case 1: case -1: dirch = (dir->y == 0 ? '-' : '|'); break;
		case 2: case -2: dirch = '\\';
		break;
	}
	pos = *start;
	hit_hero = (start != &hero);
	used = FALSE;
	changed = FALSE;
	for (i = 0; i < BOLT_LENGTH && !used; i++) {
		pos.y += dir->y;
		pos.x += dir->x;
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
			changed = FALSE;
			dir->y = -dir->y;
			dir->x = -dir->x;
			i--;
			msg("the %s bounces", name);
			break;
		default:
			if (!hit_hero && (tp = moat(pos.y, pos.x)) != NULL) {
				hit_hero = TRUE;
				changed = !changed;
				if (tp->t_oldch != '@')
					tp->t_oldch = chat(pos.y, pos.x);
				if (!save_throw(VS_MAGIC, tp) || is_frost) {
					bolt.o_pos = pos;
					used = TRUE;
					if (tp->t_type == 'D' && strcmp(name, "flame") == 0)
						msg("the flame bounces off the dragon");
					else {
						hit_monster(unc(pos), &bolt);
						if (display().tile_at(pos) != dirch)
							spotpos[i].s_under = display().tile_at(pos);
					}
				} else if (ch != 'X' || tp->t_disguise == 'X') {
					if (start == &hero)
						start_run(&pos);
					msg("the %s whizzes past the %s",
						name, monsters[ch-'A'].m_name);
				}
			} else if (hit_hero && (pos == hero)) {
				hit_hero = FALSE;
				changed = !changed;
				if (!save(VS_MAGIC)) {
					if (is_frost) {
						msg("You are frozen by a blast of frost%s.",
							noterse(" from the Ice Monster"));
						if (game().player.no_command < 20)
							game().player.no_command += spread(7);
					} else if ((pstats.s_hpt -= roll(6, 6)) <= 0) {
						if (start == &hero)
							death('b');
						else
							death(moat(start->y, start->x)->t_type);
					}
					used = TRUE;
					if (!is_frost)
						msg("you are hit by the %s", name);
				} else
					msg("the %s whizzes by you", name);
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
charge_str(const Item *obj)
{
	if (!obj->o_flags.test(ISKNOW))
		return "";
	return std::format(" [{} charges]", obj->o_charges);
}

}  // namespace rogue::items::effects
