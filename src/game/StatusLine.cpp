/*
 * The status line and the clock.
 *
 * io.c		1.4		(A.I. Design) 12/10/84
 */

#include "rogue.h"

namespace rogue {

namespace {

// The armor class the status line shows: the game's counts down from 11 (was AC())
constexpr int
armor_class(int ac)
{
	return -(ac - 11);
}

}  // namespace

/*
 * status:
 *	Display the important stats line.  Keep the cursor where it was.
 */
void
status()
{
	rogue::ui::Status st;
	int ac;
	rogue::Player &player = game().player;

	SIG2();

	/*
	 * The armor class shown ignores rings of protection, as it always did
	 */
	ac = player.armor_item() ? player.armor_item()->o_ac : player.body.t_stats.s_arm;

	st.level = game().level.depth;
	st.hp = player.body.t_stats.s_hpt;
	st.hp_max = player.body.t_stats.s_maxhp;
	st.str = player.body.t_stats.s_str;
	st.str_max = player.max_stats.s_str;
	st.gold = player.purse;
	st.armor = armor_class(ac);
	st.rank = he_man[player.body.t_stats.s_lvl-1];
	st.hunger = player.hungry_state;
	rogue::ui::display().draw_status(st);
}

/*
 * SIG2:
 *	Periodic status update: draws the clock in the bottom-right corner.
 *	The original also showed NUM LOCK/CAP LOCK and toggled "Fast Play" via
 *	Scroll Lock by reading keyboard LEDs through BIOS; terminals cannot
 *	report those, so faststate stays false.
 */
void
SIG2()
{
	static int bighand, littlehand;
	static long cur_time = 0;
	int showtime = false;
	long new_time = md_time();

	/*
	 * Do not update while a page (inventory, discoveries, ...) is shown
	 */
	if (display().page_open())
		return;
	if (new_time - cur_time >= 60)
	{
		TM local = md_localtime();
		bighand = local.hour % 12;
		littlehand = local.minute;
		cur_time = new_time - local.second;
		showtime = true;
	}

	if (showtime)
		rogue::ui::display().draw_clock(bighand ? bighand : 12, littlehand);
}

}  // namespace rogue
