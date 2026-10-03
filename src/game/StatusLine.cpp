/*
 * The status line and the clock.
 *
 * io.c		1.4		(A.I. Design) 12/10/84
 */

#include "game/StatusLine.hpp"

#include <bits/chrono.h>
#include <chrono>

#include "game/Game.hpp"
#include "platform/Clock.hpp"
#include "rules/Experience.hpp"
#include "ui/Display.hpp"

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
	rogue::Player &player = game().player;

	SIG2();

	/*
	 * The armor class shown ignores rings of protection, as it always did
	 */
	int ac = player.armor_item() ? player.armor_item()->ac : player.body.stats.s_arm;

	rogue::ui::Status st;
	st.level = game().level.depth;
	st.hp = player.body.stats.s_hpt;
	st.hp_max = player.body.stats.s_maxhp;
	st.str = player.body.stats.s_str;
	st.str_max = player.max_stats.s_str;
	st.gold = player.purse;
	st.armor = armor_class(ac);
	st.rank = rules::he_man[player.body.stats.s_lvl-1];
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
	// The minute the clock shows: the display's, not the game's (not saved)
	static std::chrono::sys_seconds cur_time{};
	std::chrono::sys_seconds new_time = platform::now();

	/*
	 * Do not update while a page (inventory, discoveries, ...) is shown
	 */
	if (ui::display().page_open())
		return;
	if (new_time - cur_time >= std::chrono::minutes(1))
	{
		std::chrono::local_seconds local = platform::local_time(new_time);
		std::chrono::hh_mm_ss hms{local - std::chrono::floor<std::chrono::days>(local)};
		int bighand = hms.hours().count() % 12;
		int littlehand = hms.minutes().count();
		cur_time = new_time - hms.seconds();
		rogue::ui::display().draw_clock(bighand ? bighand : 12, littlehand);
	}
}

}  // namespace rogue
