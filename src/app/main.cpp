/*
 * main:
 *	The main program, of course
 *
 * Command line:
 *   rogue++ [-r | savefile]   restore a saved game (-r: the savefile option)
 *   rogue++ -s                show the scores
 *   rogue++ -d <seed>         play the dungeon generated from <seed>
 */

#include <clocale>
#include <cstdlib>
#include <optional>
#include <string>

#include "core/Random.hpp"
#include "game/Endings.hpp"
#include "game/Game.hpp"
#include "game/GameLoop.hpp"
#include "game/Messages.hpp"
#include "game/NewGame.hpp"
#include "persistence/OptionsFile.hpp"
#include "platform/Session.hpp"
#include "rules/Durations.hpp"
#include "rules/Scheduler.hpp"
#include "ui/Display.hpp"
#include "world/LevelGenerator.hpp"

namespace rogue {
namespace {

int
run(int argc, char **argv)
{
	char *curarg;
	std::optional<std::string> savfile;
	Random::Seed seed = Random::from_clock();

	// Allow non-ASCII output in <curses.h>
	setlocale(LC_ALL, "");

	if (persistence::load_options(std::string(persistence::ENVFILE), game().options) == persistence::LoadResult::BadFormat)
		platform::fatal("rogue.opt: incorrect file format\n");
	/*
	 * Parse the screen environment variable.  if the string starts with
	 * "bw", then we force black and white mode.
	 */
	if (game().options.screen.starts_with("bw"))
		game().options.monochrome = true;
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
					platform::start_terminal(game().options.monochrome);
					game().noscore = true;
					score(0,0,0);
					platform::fatal("");
					break;
				case 'd': case 'D':
					if (argc < 2)
						platform::fatal("-d requires a seed\n");
					--argc;
					seed = static_cast<Random::Seed>(std::strtoul(*(++argv), nullptr, 0));
					break;
			}
		}
		else if (!savfile)
			savfile = curarg;
	}
	if (!savfile) {
		rng().reseed(seed);
		platform::start_terminal(game().options.monochrome);
		credits();

		init_player();			/* Set up initial player stats */
		init_things();			/* Set up probabilities of things */
		init_names();			/* Set up names of scrolls */
		init_colors();			/* Set up colors of potions */
		init_stones();			/* Set up stone settings of rings */
		init_materials();			/* Set up materials of wands */
		setup();
		ui::display().curtain_down();
		world::new_level();			/* Draw current level */
		/*
		 * Start up daemons and fuses
		 */
		rules::start_daemon(rules::Event::Doctor);
		rules::fuse(rules::Event::Swander, rules::wander_time());
		rules::start_daemon(rules::Event::Stomach);
		rules::start_daemon(rules::Event::Runners);
		msg("Hello {}{}.", game().options.name, noterse(".  Welcome to the Dungeons of Doom"));
		ui::display().curtain_up();
	}
	playit(savfile);
	return 0;
}

}  // namespace
}  // namespace rogue

int
main(int argc, char **argv)
{
	return rogue::run(argc, argv);
}
