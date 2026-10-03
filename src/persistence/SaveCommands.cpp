/*
 * The save command, and restoring a saved game
 *
 * save.c	1.32	(A.I. Design)	12/13/84
 */

/*
 * The original saved and restored a raw dump of the game's memory. Games are
 * now saved as JSON by persistence/SaveGame (see there for what is in them).
 * As in the original, saving ends the program and restoring deletes the save,
 * so a game can't be played on from the same save twice.
 */

#include "persistence/SaveCommands.hpp"

#include <cstdio>
#include <string>

#include "core/Glyphs.hpp"
#include "game/CommandDispatcher.hpp"
#include "game/Game.hpp"
#include "game/Messages.hpp"
#include "game/StatusLine.hpp"
#include "persistence/SaveGame.hpp"
#include "platform/Session.hpp"
#include "ui/Display.hpp"
#include "ui/Input.hpp"

namespace rogue::persistence {

namespace {

// The map as the screen shows it: what the rogue remembers of the level
MapView
map_view()
{
	MapView view;
	for (int r = 0; r < map_rows; r++)
		for (int x = 0; x < map_cols; x++)
			view[r][x] = {ui::display().tile_at({x, r + 1}), ui::display().tile_style_at({x, r + 1})};
	return view;
}

}  // namespace

/*
 * save_game:
 *	Ask for a file (the savefile option by default), save the game in it
 *	and leave.
 */
void
save_game()
{
	game().turn.after = false;
	msg("save file ({})? ", game().options.save_file);
	auto file = ui::input().read_line(MAXSTR - 1);
	if (!file) {
		msg("");
		return;
	}
	if (file->empty())
		*file = game().options.save_file;
	if (auto problems = pool_problems(game()); !problems.empty()) {
		msg("can't save: {}", problems.front());
		return;
	}
	if (auto saved = write_save(*file, game(), map_view()); !saved) {
		msg("can't save: {}", saved.error().detail);
		return;
	}
	platform::fatal("Saved the game in {}\n", *file);
}

/*
 * restore:
 *	Start the terminal, load the save in file, delete the file and show
 *	the game where it was left.
 */
void
restore(const std::string &file)
{
	MapView view;

	platform::start_terminal(game().options.monochrome);
	if (auto loaded = read_save(file, game(), view); !loaded)
		platform::fatal("Can't restore {}: {}\n", file, loaded.error().detail);
	if (std::remove(file.c_str()) != 0)
		platform::fatal("Can't delete {} after restoring it, so the game is not restored\n", file);
	for (int r = 0; r < map_rows; r++)
		for (int x = 0; x < map_cols; x++)
			ui::display().draw_tile({x, r + 1}, view[r][x].glyph, view[r][x].style);
	status();
	resume_saved_game();
}

}  // namespace rogue::persistence
