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

	int lv = player.body.stats.level;
	int ohp = player.body.stats.hp;
	player.quiet++;
	if (lv < 8)
	{
		if (player.quiet + (lv << 1) > 20)
			player.body.stats.hp++;
	}
	else
	if (player.quiet >= 3)
		player.body.stats.hp += rnd(lv - 7) + 1;
	if (player.wears(Hand::Left, Ring::Regeneration))
		player.body.stats.hp++;
	if (player.wears(Hand::Right, Ring::Regeneration))
		player.body.stats.hp++;
	if (ohp != player.body.stats.hp)
	{
		if (player.body.stats.hp > player.body.stats.max_hp)
			player.body.stats.hp = player.body.stats.max_hp;
		player.quiet = 0;
	}
}

}  // namespace rogue::rules
