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
 *
 *@ main() now lives in app/main.cpp
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
playit(char *sname)
{
	if (sname) {
		setup();			//@ first: the save has the terse and expert toggles
		restore(sname);
		display().show_cursor(FALSE);
	} else {
		game().player.old_pos.x = hero.x;
		game().player.old_pos.y = hero.y;
		game().player.old_room = roomin(&hero);
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
	static bool qstate = FALSE;

	/*
	 * if they try to interupt with a control C while in
	 * this routine blow them away!
	 */
	if (qstate == TRUE)
		leave();
	qstate = TRUE;
	game().message.end = 0;
	here = display().write("");  //@ where the cursor was
	display().clear_line(0);
	if (!game().options.terse)
		display().write_at(0, 0, "Do you wish to ");
	str_attr("end your quest now (%Yes/%No) ?");
	look(FALSE);
	answer = readchar();
	if (answer == 'y' || answer == 'Y') {
		display().clear_page();
		sprintf(prbuf, "You quit with %u gold pieces\n", game().player.purse);
		display().write_at(0, 0, prbuf);
		score(game().player.purse, 1, 0);
		fatal("");
	} else {
		display().clear_line(0);
		status();
		display().write_at(here.y, here.x, "");
		game().message.end = 0;
		game().turn.count = 0;
	}
	qstate = FALSE;
}

/*
 * leave:
 *	Leave quickly, but courteously
 */
void
leave()
{
	look(FALSE);
	display().clear_line(LINES - 1);
	display().clear_line(LINES - 2);
	fatal("Ok, if you want to leave that badly\n");
}
