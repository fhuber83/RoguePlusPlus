/*
 * What traps do: springing one, and falling to the next level.
 *
 * tr_name() comes from misc.c, be_trapped() and descend() from move.c.
 *
 * misc.c	1.4		(A.I. Design)	12/14/84
 * move.c	1.4 (A.I. Design)	12/22/84
 */

#include "world/Traps.hpp"

#include <string_view>
#include <utility>

#include "core/Coord.hpp"
#include "core/Glyphs.hpp"
#include "core/Maybe.hpp"
#include "entities/Creature.hpp"
#include "entities/Item.hpp"
#include "game/Endings.hpp"
#include "game/Game.hpp"
#include "game/Keyboard.hpp"
#include "game/Messages.hpp"
#include "game/Pool.hpp"
#include "items/Kinds.hpp"
#include "items/effects/Weapon.hpp"
#include "rules/Combat.hpp"
#include "rules/Durations.hpp"
#include "rules/Strength.hpp"
#include "ui/Display.hpp"
#include "world/LevelGenerator.hpp"
#include "world/Map.hpp"
#include "world/Rooms.hpp"
#include "world/Trap.hpp"

namespace rogue::world {

/*
 * tr_name:
 *	Print the name of a trap
 */
std::string_view
tr_name(Trap type)
{
	switch (type) {
	case Trap::Door:
		return "a trapdoor";
	case Trap::Bear:
		return "a beartrap";
	case Trap::Sleep:
		return "a sleeping gas trap";
	case Trap::Arrow:
		return "an arrow trap";
	case Trap::Teleport:
		return "a teleport trap";
	case Trap::Dart:
		return "a poison dart trap";
	}
	msg("wierd trap: {:d}", std::to_underlying(type));
	return "";
}

/*
 * be_trapped:
 *	The guy stepped on a trap.... Make him pay.
 */
Trap
be_trapped(Coord tc)
{
	rogue::Player &player = game().player;

	game().turn.count = game().turn.running = false;
	int index = INDEX(tc.y, tc.x);
	game().level.map[index] = TRAP;
	Trap tr = game().level.flags[index].trap();
	player.was_trapped = rogue::Trapped::Sprung;
	switch (tr) {
	case Trap::Door:
		descend("you fell into a trap!");
		break;
	case Trap::Bear:
		player.no_move += rules::bear_time();
		msg("you are caught in a bear trap");
		break;
	case Trap::Sleep:
		player.no_command += rules::sleep_time();
		player.body.t_flags.unset(CreatureFlag::Running);
		msg("a {}mist envelops you and you fall asleep",
			noterse("strange white "));
		break;
	case Trap::Arrow:
		if (rules::swing(player.body.t_stats.s_lvl-1, player.body.t_stats.s_arm, 1)) {
			player.body.t_stats.s_hpt -= roll(1, 6);
			if (player.body.t_stats.s_hpt <= 0) {
				msg("an arrow killed you");
				death('a');
			} else
				msg("oh no! An arrow shot you");
		}
		else {
			if (Maybe<Item> arrow = new_item()) {
				arrow->o_type = ItemKind::Weapon;
				arrow->set_which(WeaponType::Arrow);
				items::effects::init_weapon(*arrow, WeaponType::Arrow);
				arrow->o_count = 1;
				arrow->o_pos = player.body.t_pos;
				items::effects::fall(*arrow, false);
			}
			msg("an arrow shoots past you");
		}
		break;
	case Trap::Teleport:
		teleport();
		ui::display().draw_tile(tc, TRAP); /* since the hero's leaving, look()
						won't put it on for us */
		player.was_trapped = rogue::Trapped::Teleported;
		break;
	case Trap::Dart:
		if (rules::swing(player.body.t_stats.s_lvl+1, player.body.t_stats.s_arm, 1)) {
			player.body.t_stats.s_hpt -= roll(1, 4);
			if (player.body.t_stats.s_hpt <= 0) {
				msg("a poisoned dart killed you");
				death('d');
			}
			if (!player.wears(Ring::SustainStrength) && !rules::save(rules::SaveThrow::Poison))
				rules::chg_str(-1);
			msg("a dart just hit you in the shoulder");
		} else
			msg("a dart whizzes by your ear and vanishes");
		break;
	}
	flush_type();
	return tr;
}

/*
 * descend:
 *	Fall to the next level
 */
void
descend(std::string_view mesg)
{
	game().level.depth++;
	if (mesg.empty())
		msg(" ");
	new_level();
	msg("");
	msg("{}", mesg);
	if (!rules::save(rules::SaveThrow::Luck)) {
		msg("you are damaged by the fall");
		if ((game().player.body.t_stats.s_hpt -= roll(1,8)) <= 0)
			death('f');
	}
}

}  // namespace rogue::world
