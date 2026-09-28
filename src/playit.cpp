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

#include "rogue.h"

/*
 * endit:
 *	Exit the program abnormally.
 */
void
endit()
{
	fatal("Ok, if you want to exit that badly, I'll have to allow it\n");
}

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
		restore(*sname);
		display().show_cursor(false);
	} else {
		player.old_pos.x = player.body.t_pos.x;
		player.old_pos.y = player.body.t_pos.y;
		player.old_room = roomin(player.body.t_pos);
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
	coord here;
	unsigned char answer;
	static bool qstate = false;

	/*
	 * if they try to interupt with a control C while in
	 * this routine blow them away!
	 */
	if (qstate == true)
		leave();
	qstate = true;
	game().message.end = 0;
	here = display().write("");  // where the cursor was
	display().clear_line(0);
	if (!game().options.terse)
		display().write_at(0, 0, "Do you wish to ");
	str_attr("end your quest now (%Yes/%No) ?");
	look(false);
	answer = readchar();
	if (answer == 'y' || answer == 'Y') {
		display().clear_page();
		display().write_at(0, 0, std::format("You quit with {} gold pieces\n",
			static_cast<unsigned>(game().player.purse)));
		score(game().player.purse, 1, 0);
		fatal("");
	} else {
		display().clear_line(0);
		status();
		display().write_at(here.y, here.x, "");
		game().message.end = 0;
		game().turn.count = 0;
	}
	qstate = false;
}

/*
 * leave:
 *	Leave quickly, but courteously
 */
void
leave()
{
	look(false);
	display().clear_line(LINES - 1);
	display().clear_line(LINES - 2);
	fatal("Ok, if you want to leave that badly\n");
}
