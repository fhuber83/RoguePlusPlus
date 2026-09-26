#include "rogue.h"

namespace rogue::items::effects {

/*
 * wear:
 *	The player wants to wear something, so let him/her put it on.
 */
void
wear()
{
	Item *obj;
	std::string sp;

	if (game().player.armor != NULL) {
		msg("you are already wearing some{}.",
			noterse(".  You'll have to take it off first"));
		game().turn.after = FALSE;
		return;
	}
	if ((obj = get_item("wear", ItemKind::Armor)) == NULL)
		return;
	if (obj->o_type != ItemKind::Armor) {
		msg("you can't wear that");
		return;
	}
	waste_time();
	obj->o_flags.set(ISKNOW);
	sp = inv_name(obj, TRUE);
	game().player.armor = obj;
	msg("you are now wearing {}", sp);
}

/*
 * take_off:
 *	Get the armor off of the player's back
 */
void
take_off()
{
	Item *obj;

	if ((obj = game().player.armor) == NULL) {
		game().turn.after = FALSE;
		msg("you aren't wearing any armor");
		return;
	}
	if (!can_drop(game().player.armor))
		return;
	game().player.armor = NULL;
	msg("you used to be wearing {:c}) {}", pack_char(obj), inv_name(obj, TRUE));
}

/*
 * waste_time:
 *	Do nothing but let other things happen
 */
void
waste_time()
{
	do_daemons();
	do_fuses();
}

}  // namespace rogue::items::effects
