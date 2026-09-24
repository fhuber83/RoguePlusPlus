#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "ui/Cell.hpp"

namespace rogue::ui {

class Terminal;

/// The game's 80x25 text screen: a grid of CP437 cells with a cursor and a
/// current attribute, like the video memory PC Rogue wrote to.
///
/// Writes update the grid and, when a Terminal is connected, go straight
/// through to it. Reads come from the grid only. That makes read-back exact
/// (the game uses the screen as memory, see `mvinch`) and lets the screen run
/// without a terminal in tests.
class Screen {
public:
	static constexpr int Rows = 25;
	static constexpr int Cols = 80;

	using Snapshot = std::array<std::array<Cell, Cols>, Rows>;

	/// Connects a terminal and repaints it, or disconnects with nullptr.
	void connect(Terminal *terminal);
	Terminal *terminal() const { return terminal_; }

	// Cursor

	/// Moves the cursor. Returns false, leaving it in place, when out of range.
	bool set_cursor(int row, int col);
	int row() const { return row_; }
	int col() const { return col_; }
	/// Shows or hides the cursor and returns whether it was visible.
	bool show_cursor(bool visible);

	// Writing

	void set_attr(std::uint8_t attr) { attr_ = attr; }
	std::uint8_t attr() const { return attr_; }

	/// Writes at the cursor and advances it, wrapping at the end of a line.
	/// '\n' clears to the end of the line and moves to the next one.
	/// At the bottom-right corner the cursor stays put.
	void put(std::uint8_t ch, std::uint8_t attr);
	void put(std::uint8_t ch) { put(ch, attr_); }
	void put(std::string_view text);

	/// Draws `length` line cells (Cell::line) with the current attribute,
	/// clipped to the screen. The cursor does not move.
	void line(int row, int col, std::uint8_t ch, int length, bool vertical);

	/// Writes one cell without touching the cursor.
	void set(int row, int col, const Cell &cell);

	/// Blanks the whole screen and homes the cursor.
	void erase();
	/// Blanks from the cursor to the end of its line.
	void erase_to_eol();

	// Reading

	const Cell &at(int row, int col) const { return cells_[row][col]; }
	static bool contains(int row, int col) { return row >= 0 && row < Rows && col >= 0 && col < Cols; }

	Snapshot snapshot() const { return cells_; }
	/// Puts a snapshot back. The cursor does not move.
	void restore(const Snapshot &shot);
	void restore_row(const Snapshot &shot, int row);

	// Terminal

	/// Makes everything drawn so far visible.
	void refresh();
	void bell();
	/// Next key from the terminal (see Terminal::read_key), refreshing first.
	/// Without a terminal there is never a key.
	int read_key(int timeout_ms);

private:
	void draw(int row, int col);

	Snapshot cells_{};
	int row_ = 0;
	int col_ = 0;
	std::uint8_t attr_ = dos::Normal;
	bool cursor_visible_ = true;
	Terminal *terminal_ = nullptr;
};

/// The screen the game draws on.
Screen &screen();

} // namespace rogue::ui
