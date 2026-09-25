/*
 * main:
 *	The main program, of course
 *
 *@ Moved from main.c. Command line:
 *@   rogue++ [-r | savefile]   restore a saved game (-r: the savefile option)
 *@   rogue++ -s                show the scores
 *@   rogue++ -d <seed>         play the dungeon generated from <seed>
 */

#include <cstdlib>

#include "persistence/OptionsFile.hpp"
#include "rogue.h"

int
main(int argc, char **argv)
{
	char *curarg, *savfile=0;
	rogue::Random::Seed seed = rogue::Random::from_clock();

	//@ Allow non-ASCII output in <curses.h>
	setlocale(LC_ALL, "");

	init_ds();

	if (rogue::persistence::load_options(ENVFILE, game().options) == rogue::persistence::LoadResult::BadFormat)
		fatal("rogue.opt: incorrect file format\n");
	/*
	 * Parse the screen environment variable.  if the string starts with
	 * "bw", then we force black and white mode.
	 */
	if (strncmp(game().options.screen, "bw", 2) == 0)
		game().options.monochrome = TRUE;
	while (--argc) {
		curarg = *(++argv);
		if (*curarg == '-' || *curarg == '/')
		{
			switch(curarg[1])
			{
				case 'R': case 'r':
					 savfile = game().options.save_file;
					 break;
				case 's': case 'S':
					start_terminal();
					game().noscore = TRUE;
					score(0,0,0);
					fatal("");
					break;
				case 'd': case 'D':
					if (argc < 2)
						fatal("-d requires a seed\n");
					--argc;
					seed = static_cast<rogue::Random::Seed>(std::strtoul(*(++argv), nullptr, 0));
					break;
			}
		}
		else if (savfile == 0)
			savfile = curarg;
	}
	if (savfile == 0) {
		rogue::rng().reseed(seed);
		start_terminal();
		credits();

		init_player();			/* Set up initial player stats */
		init_things();			/* Set up probabilities of things */
		init_names();			/* Set up names of scrolls */
		init_colors();			/* Set up colors of potions */
		init_stones();			/* Set up stone settings of rings */
		init_materials();			/* Set up materials of wands */
		setup();
		display().curtain_down();
		new_level();			/* Draw current level */
		/*
		 * Start up daemons and fuses
		 */
		start_daemon(Event::Doctor);
		fuse(Event::Swander, WANDERTIME);
		start_daemon(Event::Stomach);
		start_daemon(Event::Runners);
		msg("Hello %s%s.", game().options.name, noterse(".  Welcome to the Dungeons of Doom"));
		display().curtain_up();
	}
	playit(savfile);
	return 0;
}
