/*
 * The rogue's strength: changing it within its bounds, and keeping track
 * of the highest it has been.
 *
 * chg_str() and add_str() come from misc.c.
 *
 * misc.c	1.4		(A.I. Design)	12/14/84
 */

#include "rogue.h"

namespace rogue::rules {

/*
 * chg_str:
 *	Used to modify the player's strength.  It keeps track of the
 *	highest it has been, just in case
 */
void
chg_str(int amt)
{
	rogue::Player &player = game().player;

	if (amt == 0)
		return;
	add_str(player.body.t_stats.s_str, amt);
	str_t comp = player.body.t_stats.s_str;
	if (player.wears(Hand::Left, Ring::AddStrength))
		add_str(comp, -player.ring_item(Hand::Left)->o_ac);
	if (player.wears(Hand::Right, Ring::AddStrength))
		add_str(comp, -player.ring_item(Hand::Right)->o_ac);
	if (comp > player.max_stats.s_str)
		player.max_stats.s_str = comp;
}

/*
 * add_str:
 *	Perform the actual add, checking upper and lower bound
 */
void
add_str(str_t &sp, int amt)
{
	if ((sp += amt) < 3)
		sp = 3;
	else if (sp > 31)
		sp = 31;
}

}  // namespace rogue::rules
