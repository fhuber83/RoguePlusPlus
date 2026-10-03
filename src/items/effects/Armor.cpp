#include "items/effects/Armor.hpp"

#include <optional>
#include <string>

#include "core/Maybe.hpp"
#include "entities/Item.hpp"
#include "game/Game.hpp"
#include "game/Messages.hpp"
#include "items/Identification.hpp"
#include "items/Inventory.hpp"
#include "rules/Scheduler.hpp"

namespace rogue::items::effects {

/*
 * wear:
 *	The player wants to wear something, so let him/her put it on.
 */
void
wear()
{
	if (game().player.armor_item()) {
		msg("you are already wearing some{}.",
			noterse(".  You'll have to take it off first"));
		game().turn.after = false;
		return;
	}
	Maybe<Item> obj = get_item("wear", ItemKind::Armor);
	if (!obj)
		return;
	if (obj->o_type != ItemKind::Armor) {
		msg("you can't wear that");
		return;
	}
	waste_time();
	obj->o_flags.set(ItemFlag::Known);
	std::string sp = inv_name(*obj, true);
	game().player.armor = game().pool.id_of(obj);
	msg("you are now wearing {}", sp);
}

/*
 * take_off:
 *	Get the armor off of the player's back
 */
void
take_off()
{
	Maybe<Item> obj = game().player.armor_item();
	if (!obj) {
		game().turn.after = false;
		msg("you aren't wearing any armor");
		return;
	}
	if (!can_drop(*obj))
		return;
	game().player.armor = std::nullopt;
	msg("you used to be wearing {:c}) {}", pack_char(*obj), inv_name(*obj, true));
}

/*
 * waste_time:
 *	Do nothing but let other things happen
 */
void
waste_time()
{
	rules::do_daemons();
	rules::do_fuses();
}

}  // namespace rogue::items::effects
