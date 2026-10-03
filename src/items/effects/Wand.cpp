#include "items/effects/Wand.hpp"

#include <format>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/Config.hpp"
#include "core/Coord.hpp"
#include "core/Glyphs.hpp"
#include "core/Maybe.hpp"
#include "entities/Creature.hpp"
#include "entities/Item.hpp"
#include "entities/List.hpp"
#include "entities/MonsterAI.hpp"
#include "entities/MonsterCatalog.hpp"
#include "game/Endings.hpp"
#include "game/Game.hpp"
#include "game/Messages.hpp"
#include "items/Inventory.hpp"
#include "items/ItemCatalog.hpp"
#include "items/Kinds.hpp"
#include "items/effects/Weapon.hpp"
#include "rules/Combat.hpp"
#include "ui/Display.hpp"
#include "world/Level.hpp"
#include "world/LevelGenerator.hpp"
#include "world/Room.hpp"
#include "world/RoomRef.hpp"
#include "world/Rooms.hpp"

namespace rogue::items::effects {

/*
 * fix_stick:
 *	Set up a new stick
 */
void
fix_stick(Item &cur)
{
	if (game().items.ws_type[cur.which<Stick>()] == "staff")
		cur.damage = "2d3";
	else
		cur.damage = "1d1";
	cur.thrown_damage = "1d1";

	cur.charges() = 3 + rnd(5);
	switch (cur.which<Stick>())
	{
	case Stick::Striking:
		cur.hit_plus = 100;
		cur.damage_plus = 3;
		cur.damage = "1d8";
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
	rogue::Turn &turn = game().turn;
	rogue::Player &player = game().player;

	Maybe<Item> obj = get_item("zap with", ItemKind::Stick);
	if (!obj)
		return;
	Stick which_one = obj->which<Stick>();
	if (obj->kind != ItemKind::Stick)
	{
		if (obj->enemy && obj->charges())
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
		if (player.body.flags.test(CreatureFlag::Blind))
			msg("you feel a warm glow around you");
		else
		{
			game().items.ws_know[Stick::Light] = true;
			if (game().level.room(*player.body.room).flags.test(RoomFlag::Gone))
				msg("the corridor glows and then fades");
			else
				msg("the room is lit by a shimmering blue light");
		}
		if (!game().level.room(*player.body.room).flags.test(RoomFlag::Gone))
		{
			game().level.room(*player.body.room).flags.unset(RoomFlag::Dark);
			/*
			 * Light the room and put the player back up
			 */
			world::enter_room(player.body.pos);
		}
		break;
	case Stick::DrainLife:
		/*
		 * Take away 1/2 of hero's hit points, then take it away
		 * evenly from the monsters in the room (or next to hero
		 * if he is in a passage)
		 */
		if (player.body.stats.hp < 2)
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
		int y = player.body.pos.y;
		int x = player.body.pos.x;
		while (step_ok(game().level.seen_at({x, y})))
		{
			y += turn.delta.y;
			x += turn.delta.x;
		}
		if (Maybe<Creature> tp = game().level.monster_at({x, y}))
		{
			unsigned char monster = tp->type;
			const unsigned char omonst = monster;
			if (monster == 'F')
				player.body.flags.unset(CreatureFlag::Held);
			if (which_one == Stick::Vorpal)
			{
				if (monster == obj->enemy)
				{
					msg("the {} vanishes in a puff of smoke",
						entities::monsters[monster-'A'].m_name);
					rules::killed(*tp, false);
				}
				else
					msg("you hear a maniacal chuckle in the distance.");
			}
			else if (which_one == Stick::Polymorph)
			{
				List<Item> pp = std::move(tp->pack);
				game().level.monsters.remove(*tp);
				if (entities::see_monst(*tp))
					ui::display().draw_tile({x, y}, game().level.at(y, x));
				unsigned char oldch = tp->under;
				turn.delta.y = y;
				turn.delta.x = x;
				entities::new_monster(*tp, monster = rnd(26) + 'A', turn.delta);
				if (entities::see_monst(*tp))
					ui::display().draw_tile({x, y}, monster);
				tp->under = oldch;
				tp->pack = std::move(pp);
				game().items.ws_know[Stick::Polymorph] |= (monster != omonst);
			}
			else if (which_one == Stick::Cancellation)
			{
				tp->flags.set(CreatureFlag::Cancelled);
				tp->flags.unset(CreatureFlag::Invisible|CreatureFlag::CanConfuse);
				tp->disguise = tp->type;
			}
			else
			{
				if (entities::see_monst(*tp))
					ui::display().draw_tile({x, y}, tp->under);
				if (which_one == Stick::TeleportAway)
				{
					tp->under = '@';
					Coord new_yx;
					do
					{
						int rm = world::rnd_room();
						new_yx = rnd_pos(game().level.rooms[rm]);
					}  while (!(is_floor(game().level.seen_at(new_yx))));
					tp->pos = new_yx;
					if (entities::see_monst(*tp))
						ui::display().draw_tile(tp->pos, tp->disguise);
					else if (player.body.flags.test(CreatureFlag::SeeMonst))
						ui::display().draw_tile(tp->pos, tp->disguise, ui::TileStyle::Inverse);
				}
				else /* it MUST BE at Stick::TeleportTo */
				{
					tp->pos.y = player.body.pos.y + turn.delta.y;
					tp->pos.x = player.body.pos.x + turn.delta.x;
				}
				if (tp->type == 'F')
					player.body.flags.unset(CreatureFlag::Held);
				if (tp->pos.y != y || tp->pos.x != x)
					tp->under = ui::display().tile_at(tp->pos);
			}
			tp->dest = Hero{};
			tp->flags.set(CreatureFlag::Running);
		}
	}
		break;
	case Stick::MagicMissile:
	{
		Item bolt;

		game().items.ws_know[Stick::MagicMissile] = true;
		bolt.kind = ItemKind::Missile;
		bolt.thrown_damage = "1d8";
		bolt.hit_plus = 1000;
		bolt.damage_plus = 1;
		bolt.flags = ItemFlag::Missile;
		if (player.weapon_item())
			bolt.launcher = launched_by(player.weapon_item()->which<WeaponType>());
		do_motion(bolt, turn.delta.y, turn.delta.x);
		Maybe<Creature> tp = game().level.monster_at(bolt.pos);
		if (tp && !rules::save_throw(rules::SaveThrow::Magic, *tp))
			hit_monster(bolt.pos.y, bolt.pos.x, bolt);
		else
		msg("the missle vanishes with a puff of smoke");
	}
		break;
	case Stick::Striking:
		turn.delta.y += player.body.pos.y;
		turn.delta.x += player.body.pos.x;
		if (Maybe<Creature> tp = game().level.monster_at(turn.delta))
		{
			if (rnd(20) == 0)
			{
				obj->damage = "3d8";
				obj->damage_plus = 9;
			}
			else
			{
				obj->damage = "2d8";
				obj->damage_plus = 4;
			}
			rules::fight(turn.delta, tp->type, *obj, false);
		}
		break;
	case Stick::HasteMonster:
	case Stick::SlowMonster: {
		int y = player.body.pos.y;
		int x = player.body.pos.x;
		while (step_ok(game().level.seen_at({x, y})))
		{
			y += turn.delta.y;
			x += turn.delta.x;
		}
		if (Maybe<Creature> tp = game().level.monster_at({x, y}))
		{
			if (which_one == Stick::HasteMonster)
			{
				if (tp->flags.test(CreatureFlag::Slow))
					tp->flags.unset(CreatureFlag::Slow);
				else
					tp->flags.set(CreatureFlag::Hasted);
			}
			else
			{
				if (tp->flags.test(CreatureFlag::Hasted))
					tp->flags.unset(CreatureFlag::Hasted);
				else
					tp->flags.set(CreatureFlag::Slow);
				tp->its_turn = true;
			}
			turn.delta.y = y;
			turn.delta.x = x;
			entities::start_run(turn.delta);
		}
		break;
	}
	case Stick::Lightning:
	case Stick::Fire:
	case Stick::Cold: {
		std::string_view name = which_one == Stick::Lightning ? "bolt"
			: which_one == Stick::Fire ? "flame" : "ice";
		fire_bolt(player.body.pos, turn.delta, name);
		game().items.ws_know[which_one] = true;
		break;
	}
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
	rogue::Player &player = game().player;
	world::Level &level = game().level;

	/*
	 * First cnt how many things we need to spread the hit points among
	 */
	std::optional<RoomRef> corp;
	if (level.at(player.body.pos) == DOOR)
		corp = level.passage_at(player.body.pos);
	else
		corp = std::nullopt;
	bool inpass = level.room(*player.body.room).flags.test(RoomFlag::Gone);
	std::vector<std::reference_wrapper<Creature>> drainee;
	for (Creature &mp : level.monsters)
		if (mp.room == player.body.room || mp.room == corp ||
			(inpass && level.at(mp.pos) == DOOR &&
			level.passage_at(mp.pos) == player.body.room))
			drainee.push_back(mp);
	int cnt = static_cast<int>(drainee.size());
	if (cnt == 0)
	{
		msg("you have a tingling feeling");
		return;
	}
	player.body.stats.hp /= 2;
	cnt = player.body.stats.hp / cnt + 1;
	/*
	 * Now zot all of the monsters
	 */
	for (Creature &tp : drainee)
	{
		if ((tp.stats.hp -= cnt) <= 0)
			rules::killed(tp, entities::see_monst(tp));
		else
			entities::start_run(tp.pos);
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
	unsigned char dirch = 0;
	rogue::Player &player = game().player;
	struct {
		Coord s_pos;
		unsigned char s_under;
	} spotpos[BOLT_LENGTH*2];
	Item bolt;
	const bool is_frost = (name == "frost");
	bolt.kind = ItemKind::Weapon;
	bolt.set_which(WeaponType::Flame);
	bolt.damage = bolt.thrown_damage = "6d6";
	bolt.hit_plus = 30;
	bolt.damage_plus = 0;
	w_names[WeaponType::Flame] = name;
	switch (dir.y + dir.x) {
		case 0: dirch = '/'; break;
		case 1: case -1: dirch = (dir.y == 0 ? '-' : '|'); break;
		case 2: case -2: dirch = '\\';
		break;
	}
	const bool by_hero = (start == player.body.pos);
	Coord pos = start;
	bool hit_hero = !by_hero;
	bool used = false;
	bool changed = false;
	int i = 0;
	for (; i < BOLT_LENGTH && !used; i++) {
		pos.y += dir.y;
		pos.x += dir.x;
		unsigned char ch = game().level.seen_at(pos);
		spotpos[i].s_pos = pos;
		if ((spotpos[i].s_under = ui::display().tile_at(pos)) == dirch)
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
			if (Maybe<Creature> tp = hit_hero ? Maybe<Creature>() : game().level.monster_at(pos)) {
				hit_hero = true;
				changed = !changed;
				if (tp->under != '@')
					tp->under = game().level.at(pos);
				if (!rules::save_throw(rules::SaveThrow::Magic, *tp) || is_frost) {
					bolt.pos = pos;
					used = true;
					if (tp->type == 'D' && name == "flame")
						msg("the flame bounces off the dragon");
					else {
						hit_monster(pos.y, pos.x, bolt);
						if (ui::display().tile_at(pos) != dirch)
							spotpos[i].s_under = ui::display().tile_at(pos);
					}
				} else if (ch != 'X' || tp->disguise == 'X') {
					if (by_hero)
						entities::start_run(pos);
					msg("the {} whizzes past the {}",
						name, entities::monsters[ch-'A'].m_name);
				}
			} else if (hit_hero && (pos == player.body.pos)) {
				hit_hero = false;
				changed = !changed;
				if (!rules::save(rules::SaveThrow::Magic)) {
					if (is_frost) {
						msg("You are frozen by a blast of frost{}.",
							noterse(" from the Ice Monster"));
						if (player.no_command < 20)
							player.no_command += spread(7);
					} else if ((player.body.stats.hp -= roll(6, 6)) <= 0) {
						if (by_hero)
							death('b');
						else
							death(game().level.monster_at(start)->type);
					}
					used = true;
					if (!is_frost)
						msg("you are hit by the {}", name);
				} else
					msg("the {} whizzes by you", name);
			}
			tick_pause();
			ui::display().draw_tile(pos, dirch, is_frost ? ui::TileStyle::FrostBolt : ui::TileStyle::Bolt);
			break;
		}
	}
	for (int j = 0; j < i; j++) {
		tick_pause();
		if (spotpos[j].s_under)
			ui::display().draw_tile(spotpos[j].s_pos, spotpos[j].s_under);
	}
}

/*
 * charge_str:
 *	Return an appropriate string for a wand charge
 */
std::string
charge_str(const Item &obj)
{
	if (!obj.flags.test(ItemFlag::Known))
		return "";
	return std::format(" [{} charges]", obj.charges());
}

}  // namespace rogue::items::effects
