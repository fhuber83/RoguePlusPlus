#include "items/effects/Scroll.hpp"

#include <optional>
#include <string_view>

#include "core/Coord.hpp"
#include "core/Glyphs.hpp"
#include "core/Maybe.hpp"
#include "entities/Creature.hpp"
#include "entities/Item.hpp"
#include "entities/MonsterAI.hpp"
#include "entities/MonsterCatalog.hpp"
#include "game/Game.hpp"
#include "game/Messages.hpp"
#include "game/Pool.hpp"
#include "game/StatusLine.hpp"
#include "items/Identification.hpp"
#include "items/Inventory.hpp"
#include "items/ItemCatalog.hpp"
#include "items/Kinds.hpp"
#include "items/effects/Weapon.hpp"
#include "rules/Durations.hpp"
#include "ui/Display.hpp"
#include "world/Level.hpp"
#include "world/Look.hpp"
#include "world/MapFlags.hpp"
#include "world/RoomRef.hpp"
#include "world/Rooms.hpp"

namespace rogue::items::effects {

constexpr std::string_view laugh = "you hear maniacal laughter{}.";
constexpr std::string_view in_dist = " in the distance";
/*
 * read_scroll:
 *	Read a scroll from the pack and do the appropriate thing
 */
void
read_scroll()
{
	bool discardit = false;
	rogue::Player &player = game().player;
	world::Level &level = game().level;
	rogue::Items &items = game().items;

	Maybe<Item> obj = get_item("read", ItemKind::Scroll);
	if (!obj)
		return;
	if (obj->o_type != ItemKind::Scroll){
		msg("there is nothing on it to read");
		return;
	}
	ifterse("the scroll vanishes","as you read the scroll, it vanishes");
	/*
	 * Calculate the effect it has on the poor guy.
	 */
	if (obj == player.weapon_item())
		player.weapon = std::nullopt;
	switch (obj->which<Scroll>()){
	case Scroll::MonsterConfusion:
		/*
		 * Scroll of monster confusion.  Give him that power.
		 */
		player.body.flags.set(CreatureFlag::CanConfuse);
		msg("your hands begin to glow red");
		break;
	case Scroll::EnchantArmor:
		if (player.armor_item()) {
			player.armor_item()->o_ac--;
			player.armor_item()->o_flags.unset(ItemFlag::Cursed);
			ifterse("your armor glows faintly",
				"your armor glows faintly for a moment");
		}
		break;
	case Scroll::HoldMonster:
		/*
		 * Hold monster scroll.  Stop all monsters within two spaces
		 * from chasing after the hero.
		 */

		for (int x = player.body.pos.x - 3; x <= player.body.pos.x + 3; x++)
			if (x >= 0 && x < MAXCOLS)
				for (int y = player.body.pos.y - 3; y <= player.body.pos.y + 3; y++)
					if (y > 0 && y < maxrow)
						if (Maybe<Creature> mo = level.monster_at({x, y})) {
							mo->flags.unset(CreatureFlag::Running);
							mo->flags.set(CreatureFlag::Held);
						}
		break;
	case Scroll::Sleep:
		/*
		 * Scroll which makes you fall asleep
		 */
		items.s_know[Scroll::Sleep] = true;
		player.no_command += rnd(rules::sleep_time()) + 4;
		player.body.flags.unset(CreatureFlag::Running);
		msg("you fall asleep");
		break;
	case Scroll::CreateMonster:
		{
		std::optional<Coord> mp = entities::plop_monster(player.body.pos.y, player.body.pos.x);

		Maybe<Creature> mo = mp ? new_creature() : Maybe<Creature>();
		if (mo)
			entities::new_monster(*mo, entities::randmonster(false), *mp);
		else
			ifterse("you hear a faint cry of anguish",
				"you hear a faint cry of anguish in the distance");
		}
		break;
	case Scroll::Identify:
		/*
		 * Identify, let the rogue figure something out
		 */
		items.s_know[Scroll::Identify] = true;
		msg("this scroll is an identify scroll");
		if (game().options.menu == "on" || game().options.menu == "sel")
			more(" More ");
		whatis();
		break;
	case Scroll::MagicMapping:
		/*
		 * Scroll of magic mapping.
		 */
		items.s_know[Scroll::MagicMapping] = true;
		msg("oh, now this scroll has a map on it");
		/*
		 * Take all the things we want to keep hidden out of the window
		 */
		for (int y = 1; y < maxrow; y++)
			for (int x = 0; x < MAXCOLS; x++) {
				int index = world::Level::index({x, y});
				unsigned char ch = level.map[index];
				switch (ch)
				{
				case VWALL:
				case HWALL:
				case ULWALL:
				case URWALL:
				case LLWALL:
				case LRWALL:
					if (!level.flags[index].test(MapFlag::Real)) {
						ch = level.map[index] = DOOR;
						level.flags[index].unset(MapFlag::Real);
					}
					/* fallthrough */
				case DOOR:
				case PASSAGE:
				case STAIRS:
					if (Maybe<Creature> mo = level.monster_at({x, y}))
						if (mo->under == ' ')
							mo->under = ch;
					break;
				default:
					ch = ' ';
				}
				if (ch != ' ')
					ui::display().draw_tile({x, y}, ch,
							(ch == DOOR && ui::display().tile_at({x, y}) != DOOR)
								? ui::TileStyle::Inverse : ui::TileStyle::Normal);
			}
		break;
	case Scroll::FoodDetection: {
		/*
		 * Scroll of food detection
		 */
		bool found = false;
		for (Item &op : level.objects) {
			if (op.o_type == ItemKind::Food) {
				found = true;
				ui::display().draw_tile(op.o_pos, FOOD, ui::TileStyle::Inverse);
			} else /* as a bonus this will detect amulets as well */
			if (op.o_type == ItemKind::Amulet) {
				found = true;
				ui::display().draw_tile(op.o_pos, AMULET, ui::TileStyle::Inverse);
			}
		}
		if (found) {
			items.s_know[Scroll::FoodDetection] = true;
			msg("your nose tingles as you sense food");
		} else
			ifterse("you hear a growling noise close by","you hear a growling noise very close to you");
		break;
	}
	case Scroll::Teleportation:
		/*
		 * Scroll of teleportation:
		 * Make him dissapear and reappear
		 */
		{
		std::optional<RoomRef> cur_room = player.body.room;
		world::teleport();
		if (cur_room != player.body.room)
			items.s_know[Scroll::Teleportation] = true;
		}
		break;
	case Scroll::EnchantWeapon:
		if (!player.weapon_item() || player.weapon_item()->o_type != ItemKind::Weapon)
		msg("you feel a strange sense of loss");
		else
		{
		player.weapon_item()->o_flags.unset(ItemFlag::Cursed);
		if (rnd(2) == 0)
			player.weapon_item()->o_hplus++;
		else
			player.weapon_item()->o_dplus++;
		ifterse("your {} glows blue","your {} glows blue for a moment", w_names[player.weapon_item()->which<WeaponType>()]);
		}
		break;
	case Scroll::ScareMonster:
		/*
		 * Reading it is a mistake and produces laughter at the
		 * poor rogue's boo boo.
		 */
			msg(laugh, game().options.brief() ? "" : in_dist);
		break;
	case Scroll::RemoveCurse:
		if (player.armor_item())
			player.armor_item()->o_flags.unset(ItemFlag::Cursed);
		if (player.weapon_item())
			player.weapon_item()->o_flags.unset(ItemFlag::Cursed);
		if (player.ring_item(Hand::Left))
			player.ring_item(Hand::Left)->o_flags.unset(ItemFlag::Cursed);
		if (player.ring_item(Hand::Right))
			player.ring_item(Hand::Right)->o_flags.unset(ItemFlag::Cursed);
		ifterse("somebody is watching over you","you feel as if somebody is watching over you");
		break;
	case Scroll::AggravateMonsters:
		/*
		 * This scroll aggravates all the monsters on the current
		 * level and sets them running towards the hero
		 */
		entities::aggravate();
		ifterse("you hear a humming noise",
					"you hear a high pitched humming noise");
		break;
	case Scroll::BlankPaper:
		msg("this scroll seems to be blank");
		break;
	case Scroll::Vorpalize:
		/*
		 * Extra Vorpal Enchant Weapon
		 *     Give weapon +1,+1
		 *     Is extremely vorpal against one certain type of monster
		 *     Against this type (o_enemy) the weapon gets:
		 *		+4,+4
		 *		The ability to zap one such monster into oblivion
		 *
		 *     Some of these are cursed and if the rogue misses her saving
		 *     throw she will be forced to attack monsters of this type
		 *     whenever she sees one (not yet implemented)
		 *
		 * If he doesn't have a weapon I get to chortle again!
		 */
		if (!player.weapon_item() || player.weapon_item()->o_type != ItemKind::Weapon)
			msg(laugh, game().options.brief() ? "" : in_dist);
		else {
			/*
			 * You aren't allowed to doubly vorpalize a weapon.
			 */
			if (player.weapon_item()->o_enemy != 0) {
				msg("your {} vanishes in a puff of smoke",
				w_names[player.weapon_item()->which<WeaponType>()]);
				player.body.pack.remove(*player.weapon_item());
				discard(*player.weapon_item());
				player.weapon = std::nullopt;
			} else {
				player.weapon_item()->o_enemy = entities::pick_mons();
				player.weapon_item()->o_hplus++;
				player.weapon_item()->o_dplus++;
				player.weapon_item()->charges() = 1;
				msg(flashmsg, w_names[player.weapon_item()->which<WeaponType>()],
					game().options.brief() ? "" : intense);

				/*
				 * Sometimes this is a mixed blessing ...
					if (rnd(20) == 0) {
						cur_weapon->o_flags.set(ItemFlag::Cursed);
						if (!save(SaveThrow::Magic)) {
							cur_weapon->o_flags.set(ItemFlag::Ego|ItemFlag::Revealed);
							s_know[Scroll::Vorpalize] = true;
							msg("you feel a sudden desire to kill {}s.",
							monsters[cur_weapon->o_enemy-'A'].m_name);
						}
					}
				 */
			}
		}
		break;
	default:
		msg("what a puzzling scroll!");
		return;
	}
	world::look(true);	/* put the result of the scroll on the screen */
	status();
	/*
	 * Get rid of the thing
	 */
	player.in_pack--;
	if (obj->o_count > 1)
	obj->o_count--;
	else
	{
	player.body.pack.remove(*obj);
	discardit = true;
	}
	call_it(items.s_know[obj->which<Scroll>()], items.s_guess[obj->which<Scroll>()]);

	if (discardit)
	discard(*obj);
}

}  // namespace rogue::items::effects
