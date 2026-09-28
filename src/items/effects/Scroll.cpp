#include "rogue.h"

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
	Item *obj;
	int y, x;
	unsigned char ch;
	Item *op;
	Creature *mo;
	int index;
	bool discardit = false;
	rogue::Player &player = game().player;
	rogue::Level &level = game().level;
	rogue::Items &items = game().items;

	obj = get_item("read", ItemKind::Scroll);
	if (obj == nullptr)
		return;
	if (obj->o_type != ItemKind::Scroll){
		msg("there is nothing on it to read");
		return;
	}
	ifterse("the scroll vanishes","as you read the scroll, it vanishes");
	/*
	 * Calculate the effect it has on the poor guy.
	 */
	if (obj == player.weapon)
		player.weapon = nullptr;
	switch (obj->which<Scroll>()){
	case Scroll::MonsterConfusion:
		/*
		 * Scroll of monster confusion.  Give him that power.
		 */
		player.body.t_flags.set(CANHUH);
		msg("your hands begin to glow red");
		break;
	case Scroll::EnchantArmor:
		if (player.armor != nullptr) {
			player.armor->o_ac--;
			player.armor->o_flags.unset(ISCURSED);
			ifterse("your armor glows faintly",
				"your armor glows faintly for a moment");
		}
		break;
	case Scroll::HoldMonster:
		/*
		 * Hold monster scroll.  Stop all monsters within two spaces
		 * from chasing after the hero.
		 */

		for (x = player.body.t_pos.x - 3; x <= player.body.t_pos.x + 3; x++)
			if (x >= 0 && x < COLS)
				for (y = player.body.t_pos.y - 3; y <= player.body.t_pos.y + 3; y++)
					if ((y > 0 && y < maxrow) && ((mo=moat(y, x)) != nullptr)) {
						mo->t_flags.unset(ISRUN);
						mo->t_flags.set(ISHELD);
					}
		break;
	case Scroll::Sleep:
		/*
		 * Scroll which makes you fall asleep
		 */
		items.s_know[Scroll::Sleep] = true;
		player.no_command += rnd(sleep_time()) + 4;
		player.body.t_flags.unset(ISRUN);
		msg("you fall asleep");
		break;
	case Scroll::CreateMonster:
		{
		std::optional<Coord> mp = plop_monster(player.body.t_pos.y, player.body.t_pos.x);

		if (mp && (mo=new_creature()) != nullptr)
			new_monster(mo, randmonster(false), *mp);
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
		for (y = 1; y < maxrow; y++)
			for (x = 0; x < COLS; x++) {
				index = INDEX(y, x);
				switch (ch = level.map[index])
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
					if ((mo = moat(y, x)) != nullptr)
						if (mo->t_oldch == ' ')
							mo->t_oldch = ch;
					break;
				default:
					ch = ' ';
				}
				if (ch != ' ')
					display().draw_tile({x, y}, ch,
							(ch == DOOR && display().tile_at({x, y}) != DOOR)
								? TileStyle::Inverse : TileStyle::Normal);
			}
		break;
	case Scroll::FoodDetection:
		/*
		 * Scroll of food detection
		 */
		ch = false;
		for (op = level.objects.first(); op != nullptr; op = level.objects.after(op)) {
			if (op->o_type == ItemKind::Food) {
				ch = true;
				display().draw_tile(op->o_pos, FOOD, TileStyle::Inverse);
			} else /* as a bonus this will detect amulets as well */
			if (op->o_type == ItemKind::Amulet) {
				ch = true;
				display().draw_tile(op->o_pos, AMULET, TileStyle::Inverse);
			}
		}
		if (ch) {
			items.s_know[Scroll::FoodDetection] = true;
			msg("your nose tingles as you sense food");
		} else
			ifterse("you hear a growling noise close by","you hear a growling noise very close to you");
		break;
	case Scroll::Teleportation:
		/*
		 * Scroll of teleportation:
		 * Make him dissapear and reappear
		 */
		{
		struct room *cur_room;

		cur_room = player.body.t_room;
		teleport();
		if (cur_room != player.body.t_room)
			items.s_know[Scroll::Teleportation] = true;
		}
		break;
	case Scroll::EnchantWeapon:
		if (player.weapon == nullptr || player.weapon->o_type != ItemKind::Weapon)
		msg("you feel a strange sense of loss");
		else
		{
		player.weapon->o_flags.unset(ISCURSED);
		if (rnd(2) == 0)
			player.weapon->o_hplus++;
		else
			player.weapon->o_dplus++;
		ifterse("your {} glows blue","your {} glows blue for a moment", w_names[player.weapon->which<WeaponType>()]);
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
		if (player.armor != nullptr)
			player.armor->o_flags.unset(ISCURSED);
		if (player.weapon != nullptr)
			player.weapon->o_flags.unset(ISCURSED);
		if (player.rings[Hand::Left] != nullptr)
			player.rings[Hand::Left]->o_flags.unset(ISCURSED);
		if (player.rings[Hand::Right] != nullptr)
			player.rings[Hand::Right]->o_flags.unset(ISCURSED);
		ifterse("somebody is watching over you","you feel as if somebody is watching over you");
		break;
	case Scroll::AggravateMonsters:
		/*
		 * This scroll aggravates all the monsters on the current
		 * level and sets them running towards the hero
		 */
		aggravate();
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
		if (player.weapon == nullptr || player.weapon->o_type != ItemKind::Weapon)
			msg(laugh, game().options.brief() ? "" : in_dist);
		else {
			/*
			 * You aren't allowed to doubly vorpalize a weapon.
			 */
			if (player.weapon->o_enemy != 0) {
				msg("your {} vanishes in a puff of smoke",
				w_names[player.weapon->which<WeaponType>()]);
				player.body.t_pack.remove(player.weapon);
				discard(player.weapon);
				player.weapon = nullptr;
			} else {
				player.weapon->o_enemy = pick_mons();
				player.weapon->o_hplus++;
				player.weapon->o_dplus++;
				player.weapon->charges() = 1;
				msg(flashmsg, w_names[player.weapon->which<WeaponType>()],
					game().options.brief() ? "" : intense);

				/*
				 * Sometimes this is a mixed blessing ...
					if (rnd(20) == 0) {
						cur_weapon->o_flags.set(ISCURSED);
						if (!save(SaveThrow::Magic)) {
							cur_weapon->o_flags.set(ISEGO|ISREVEAL);
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
	look(true);	/* put the result of the scroll on the screen */
	status();
	/*
	 * Get rid of the thing
	 */
	player.in_pack--;
	if (obj->o_count > 1)
	obj->o_count--;
	else
	{
	player.body.t_pack.remove(obj);
	discardit = true;
	}
	call_it(items.s_know[obj->which<Scroll>()], items.s_guess[obj->which<Scroll>()]);

	if (discardit)
	discard(obj);
}

}  // namespace rogue::items::effects
