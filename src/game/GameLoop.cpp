/*
 * ###   ###   ###  #   # #####
 * #  # #   # #   # #   # #
 * #  # #   # #   # #   # #
 * ###  #   # #     #   # ###
 * #  # #   # #  ## #   # #
 * #  # #   # #   # #   # #
 * #  #  ###   ###   ###  #####
 *
 * Exploring the Dungeons of Doom
 * Copyright (C) 1981 by Michael Toy, Ken Arnold, and Glenn Wichman
 * main.c	1.4 (A.I. Design) 11/28/84
 * All rights reserved
 * Copyright (C) 1983 by Mel Sibony, Jon Lane (AI Design update for the IBMPC)
 */

#include "game/GameLoop.hpp"

#include <format>
#include <optional>
#include <string>

#include "core/Coord.hpp"
#include "game/CommandDispatcher.hpp"
#include "game/Endings.hpp"
#include "game/Game.hpp"
#include "game/Keyboard.hpp"
#include "game/Messages.hpp"
#include "game/NewGame.hpp"
#include "game/StatusLine.hpp"
#include "persistence/SaveCommands.hpp"
#include "platform/Session.hpp"
#include "ui/Display.hpp"
#include "world/Look.hpp"
#include "world/Rooms.hpp"

namespace rogue {

namespace {

/*
 * endit:
 *	Exit the program abnormally.
 */
void
endit()
{
	platform::fatal("Ok, if you want to exit that badly, I'll have to allow it\n");
}

}  // namespace

/*
 * playit:
 *	The main loop of the program.  Loop until the game is over,
 *	refreshing things and looking at the proper times.
 */
void
playit(const std::optional<std::string> &sname)
{
	rogue::Player &player = game().player;

	if (sname) {
		setup();			// first: the save has the terse and expert toggles
		persistence::restore(*sname);
		ui::display().show_cursor(false);
	} else {
		player.old_pos.x = player.body.pos.x;
		player.old_pos.y = player.body.pos.y;
		player.old_room = world::roomin(player.body.pos);
	}
	while (game().playing)
		command();			/* Command execution */
	endit();
}

/*
 * quit:
 *	Have player make certain, then exit.
 */
void
quit()
{
	game().message.end = 0;
	Coord here = ui::display().write("");  // where the cursor was
	ui::display().clear_line(0);
	if (!game().options.terse)
		ui::display().write_at(0, 0, "Do you wish to ");
	str_attr("end your quest now (%Yes/%No) ?");
	world::look(false);
	unsigned char answer = readchar();
	if (answer == 'y' || answer == 'Y') {
		ui::display().clear_page();
		ui::display().write_at(0, 0, std::format("You quit with {} gold pieces\n",
			static_cast<unsigned>(game().player.purse)));
		score(game().player.purse, 1, 0);
		platform::fatal("");
	} else {
		ui::display().clear_line(0);
		status();
		ui::display().write_at(here.y, here.x, "");
		game().message.end = 0;
		game().turn.count = 0;
	}
}

}  // namespace rogue
