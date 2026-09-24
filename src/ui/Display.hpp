#pragma once

#include <cstdint>
#include <string_view>

#include "core/Coord.hpp"

namespace rogue::ui {

/// How a map tile is drawn, besides its glyph.
enum class TileStyle : std::uint8_t {
	Normal,
	Inverse,  ///< passages and mazes, sensed monsters, detected items
	Bolt,      ///< a magic bolt (fire, lightning) in flight
	FrostBolt, ///< a bolt of frost in flight
};

/// What the status lines at the bottom of the screen show.
struct Status {
	int level = 0;
	int hp = 0;
	int hp_max = 0;
	unsigned str = 0;
	unsigned str_max = 0;
	int gold = 0;
	int armor = 0;         ///< armour class as shown (higher is better)
	std::string_view rank; ///< experience title, "Guild Novice" etc.
	int hunger = 0;        ///< 0 fed, 1 hungry, 2 weak, 3 faint
};

/// How the game shows things to the player. Game logic decides what to
/// show and when to wait for keys; a Display only draws.
class Display {
public:
	virtual ~Display() = default;

	// Message line

	/// Replaces the message line with `text`, cut at the screen width.
	/// Leaves the cursor after the text.
	virtual void draw_message(std::string_view text) = 0;
	/// Blanks the message line and puts the cursor at its start.
	virtual void clear_message() = 0;

	/// Shows `prompt` (" More ", " Cont ") after a message that ends in
	/// column `col`. If it does not fit, it covers the end of the line
	/// instead, and blink_more() alternates it with what it covers.
	virtual void show_more(std::string_view prompt, int col) = 0;
	virtual void blink_more() = 0;
	/// Removes the prompt again.
	virtual void hide_more() = 0;

	// Map

	/// Draws `glyph` (a CP437 code, for now) at a map position. Positions
	/// off the screen are ignored.
	virtual void draw_tile(Coord pos, std::uint8_t glyph, TileStyle style = TileStyle::Normal) = 0;
	/// The glyph last drawn at a map position: the hero's view of the level.
	virtual std::uint8_t tile_at(Coord pos) const = 0;

	// Status lines

	/// Redraws the fields that changed since the last call.
	/// The cursor does not move.
	virtual void draw_status(const Status &status) = 0;
	/// Shows the time (12-hour clock). The cursor does not move.
	virtual void draw_clock(int hour, int minute) = 0;
	/// Shows the repeat count typed before a command, or blanks it for 0.
	virtual void draw_count(int count) = 0;

	// Output

	/// Makes everything drawn so far visible, for animations.
	virtual void flush() = 0;
	virtual void bell() = 0;
};

/// The display the game uses.
Display &display();

} // namespace rogue::ui
