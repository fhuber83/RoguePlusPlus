/*
 * Regaining hit points: the doctor daemon.
 *
 * @(#)daemons.c	5.1 (Berkeley) 5/11/82
 */

#include "rules/Regeneration.hpp"

#include "game/Game.hpp"
#include "items/Kinds.hpp"

namespace rogue::rules {

/*
 * doctor:
 *	A healing daemon that restores hit points after rest
 */
void
doctor()
{
	rogue::Player &player = game().player;

	int lv = player.body.t_stats.s_lvl;
	int ohp = player.body.t_stats.s_hpt;
	player.quiet++;
	if (lv < 8)
	{
		if (player.quiet + (lv << 1) > 20)
			player.body.t_stats.s_hpt++;
	}
	else
	if (player.quiet >= 3)
		player.body.t_stats.s_hpt += rnd(lv - 7) + 1;
	if (player.wears(Hand::Left, Ring::Regeneration))
		player.body.t_stats.s_hpt++;
	if (player.wears(Hand::Right, Ring::Regeneration))
		player.body.t_stats.s_hpt++;
	if (ohp != player.body.t_stats.s_hpt)
	{
		if (player.body.t_stats.s_hpt > player.body.t_stats.s_maxhp)
			player.body.t_stats.s_hpt = player.body.t_stats.s_maxhp;
		player.quiet = 0;
	}
}

}  // namespace rogue::rules
