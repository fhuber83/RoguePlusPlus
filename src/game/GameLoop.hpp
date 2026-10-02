#pragma once

#include <optional>
#include <string>

/*
 * The main loop, and quitting.
 */

namespace rogue {

/*
 * playit:
 *	Play the game set up by main(), or restore the one saved in sname,
 *	one command at a time until it ends.
 */
void playit(const std::optional<std::string> &sname);

/*
 * quit:
 *	Have the player make certain, then end the game with a score.
 */
void quit();

}  // namespace rogue
