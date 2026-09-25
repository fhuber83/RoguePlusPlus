#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

#include "ui/Display.hpp"

/*
 * Saved games: the whole of rogue::Game, plus the map as the screen shows
 * it (the rogue's memory of the level, which game code reads back), as JSON:
 *
 *	{ "format": "rogue++ save", "version": 1, "random": {...},
 *	  "options": {...}, "player": {...}, "level": {...}, "items": {...},
 *	  "pool": {...}, "scheduler": [...], "message": {...}, "turn": {...},
 *	  ..., "screen": {...} }
 *
 * Pointers are saved as what they point to:
 *	- items and creatures by pool slot (slots keep their numbers, since new
 *	  things take the first free one);
 *	- a monster's t_dest as "hero", {"room_gold": i}, {"passage_gold": i}
 *	  or {"item": slot} (a floor item's position);
 *	- rooms as {"room": i} or {"passage": i};
 *	- damage texts, potion colours, stones, materials and macro typeahead
 *	  by their text, which the loader keeps for the rest of the program;
 *	  a venus flytrap's damage is {"alias": "flytrap"}, since it is the
 *	  rogue's flytrap_damage buffer, which grows;
 *	- guesses by their index in Items::guesses.
 * Bytes of game text are written as the code points of the same values
 * (see ByteText.hpp). The map, its flags and the screen are rows of hex.
 *
 * Of the options only the game's own are saved (name, fruit, terse,
 * expert); the rest come from rogue.opt when the game is restored.
 */

namespace rogue {
struct Game;
}

namespace rogue::persistence {

// The map part of the screen: rows 1 to maxrow - 1, all columns
inline constexpr int map_rows = 22;
inline constexpr int map_cols = 80;

struct MapTile {
	std::uint8_t glyph = ' ';
	ui::TileStyle style = ui::TileStyle::Normal;
	friend bool operator==(const MapTile &, const MapTile &) = default;
};

// view[r][x] is the tile at screen row r + 1, column x
using MapView = std::array<std::array<MapTile, map_cols>, map_rows>;

struct SaveError {
	enum class Kind {
		Unreadable,			// The file can't be opened, read or written
		BadFormat,			// Not a save, or a value is missing or wrong
		WrongVersion,		// A save from another version of the format
		Inconsistent,		// Loaded, but the pool doesn't add up (see pool_problems)
	};
	Kind kind;
	std::string detail;		// What was wrong, for the player
};

/*
 * format_save:
 *	The JSON text of a save of game, with the map as view shows it.
 */
std::string format_save(const Game &game, const MapView &view);

/*
 * parse_save:
 *	Load a save into game and view. On failure game is left half loaded;
 *	restoring only happens at startup, where that ends the program.
 */
std::expected<void, SaveError> parse_save(std::string_view text, Game &game, MapView &view);

/*
 * write_save:
 *	Write a save to path, through a temporary file renamed over it.
 */
std::expected<void, SaveError> write_save(const char *path, const Game &game, const MapView &view);

/*
 * read_save:
 *	Read and load the save at path.
 */
std::expected<void, SaveError> read_save(const char *path, Game &game, MapView &view);

}  // namespace rogue::persistence
