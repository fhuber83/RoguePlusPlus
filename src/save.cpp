/*
 * save and restore routines
 *
 * save.c	1.32	(A.I. Design)	12/13/84
 */

/*
 * The original saved and restored a raw dump of the game's memory. Games are
 * now saved as JSON by persistence/SaveGame (see there for what is in them).
 * As in the original, saving ends the program and restoring deletes the save,
 * so a game can't be played on from the same save twice.
 */

#include <cstdio>
#include <string>

#include "persistence/SaveGame.hpp"
#include "rogue.h"

using rogue::persistence::MapView;

// The map as the screen shows it: what the rogue remembers of the level
static MapView
map_view()
{
	MapView view;
	for (int r = 0; r < rogue::persistence::map_rows; r++)
		for (int x = 0; x < rogue::persistence::map_cols; x++)
			view[r][x] = {display().tile_at({x, r + 1}), display().tile_style_at({x, r + 1})};
	return view;
}

/*
 * save_game:
 *	Ask for a file (the savefile option by default), save the game in it
 *	and leave.
 */
void
save_game()
{
	char file[MAXSTR];

	game().turn.after = FALSE;
	msg("save file ({})? ", game().options.save_file);
	if (input().read_line(file, sizeof file - 1) == ESCAPE) {
		msg("");
		return;
	}
	if (*file == '\0')
		strcpy(file, game().options.save_file);
	if (auto problems = rogue::pool_problems(game()); !problems.empty()) {
		msg("can't save: {}", problems.front());
		return;
	}
	if (auto saved = rogue::persistence::write_save(file, game(), map_view()); !saved) {
		msg("can't save: {}", saved.error().detail);
		return;
	}
	fatal("Saved the game in {}\n", file);
}

/*
 * restore:
 *	Start the terminal, load the save in file, delete the file and show
 *	the game where it was left.
 */
void
restore(char *file)
{
	MapView view;

	start_terminal();
	if (auto loaded = rogue::persistence::read_save(file, game(), view); !loaded)
		fatal("Can't restore {}: {}\n", file, loaded.error().detail);
	if (std::remove(file) != 0)
		fatal("Can't delete {} after restoring it, so the game is not restored\n", file);
	for (int r = 0; r < rogue::persistence::map_rows; r++)
		for (int x = 0; x < rogue::persistence::map_cols; x++)
			display().draw_tile({x, r + 1}, view[r][x].glyph, view[r][x].style);
	status();
	rogue::resume_saved_game();
}
