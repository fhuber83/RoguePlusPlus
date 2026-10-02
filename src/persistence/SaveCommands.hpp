#pragma once

#include <string>

/*
 * Saving a game with the S command, and restoring one. The file itself is
 * persistence/SaveGame.
 */

namespace rogue::persistence {

/*
 * save_game:
 *	Ask for a file (the savefile option by default), save the game in it
 *	and leave. Stays in the game if the player escapes or it can't save.
 */
void save_game();

/*
 * restore:
 *	Start the terminal, load the save in file, delete the file and show
 *	the game where it was left. Exits if any of it fails.
 */
void restore(const std::string &file);

}  // namespace rogue::persistence
