#pragma once

#include <cstdint>
#include <expected>
#include <span>
#include <string>
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

/// Text styles for pages and prompts. They are the original's colour
/// macros by name (monochrome screens map most of them to Normal).
enum class Ink : std::uint8_t {
	Normal,
	Reverse,
	Bold,
	Bright,
	Underline,
	Red,
	Green,
	Brown,
	Yellow,
	Blue,
	LightMagenta,
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

/// One line of the Hall of Fame.
struct ScoreLine {
	int gold = 0;
	std::string_view name;
	std::string_view text; ///< rank and fate: ` "Fighter" killed by a bat on level 3`
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
	/// A style that draws the tile at a map position as it is now (Normal
	/// if none does). Where two styles look the same, either may come back.
	virtual TileStyle tile_style_at(Coord pos) const = 0;

	// Status lines

	/// Redraws the fields that changed since the last call.
	/// The cursor does not move.
	virtual void draw_status(const Status &status) = 0;
	/// Shows the time (12-hour clock). The cursor does not move.
	virtual void draw_clock(int hour, int minute) = 0;
	/// Shows the repeat count typed before a command, or blanks it for 0.
	virtual void draw_count(int count) = 0;

	// Pages: full-screen text over the game view (help, inventory, ...)

	/// Keeps the game view to bring back later.
	virtual void open_page() = 0;
	/// Brings the game view back.
	virtual void close_page() = 0;
	/// Whether a page is open (the clock is not drawn meanwhile).
	virtual bool page_open() const = 0;
	/// Blanks the whole screen.
	virtual void clear_page() = 0;

	// Text

	/// Writes `text` at (row, col), wrapping at the right edge; '\n' ends a
	/// line. Returns where the text ended. An empty text only moves there.
	virtual Coord write_at(int row, int col, std::string_view text, Ink ink = Ink::Normal) = 0;
	/// Writes where the last text ended.
	virtual Coord write(std::string_view text, Ink ink = Ink::Normal) = 0;
	/// Blanks a line from `col` to the right edge.
	virtual void clear_line(int row, int col = 0) = 0;
	/// Shows or hides the text cursor and returns whether it was shown.
	virtual bool show_cursor(bool visible) = 0;

	// Title and ending screens

	/// The credits screen with the name prompt. Leaves the cursor where the
	/// name goes, with Ink::Bright for the typing.
	virtual void draw_title() = 0;
	/// Removes the name prompt from the title screen.
	virtual void end_title() = 0;
	virtual void draw_tombstone(std::string_view name, std::string_view killer, int gold, int year) = 0;
	/// The Hall of Fame; `highlight` is the index of the new entry, or -1.
	virtual void draw_scores(std::span<const ScoreLine> lines, int highlight) = 0;
	/// The "You made it!" screen, `brief` for terse mode, with a prompt to
	/// continue.
	virtual void draw_winner(bool brief) = 0;

	// Transitions

	/// Lowers a curtain and blanks the screen behind it. Nothing drawn
	/// afterwards shows until curtain_up().
	virtual void curtain_down() = 0;
	/// Raises the curtain on what was drawn behind it.
	virtual void curtain_up() = 0;
	/// Clears the screen with the shrinking-boxes effect (new level).
	virtual void wipe() = 0;

	// Output


	/// Makes everything drawn so far visible, for animations.
	virtual void flush() = 0;
	virtual void bell() = 0;
};

/// The display the game uses.
Display &display();

/// Starts the terminal the display shows on; `monochrome` turns colours
/// off. Fails when the terminal is too small.
std::expected<void, std::string> start_terminal(bool monochrome);
/// Gives the terminal back to the shell. Safe to call when not started.
void stop_terminal();

} // namespace rogue::ui
