#pragma once

#include <expected>
#include <string>

#include "ui/Terminal.hpp"

namespace rogue::ui {

/// Terminal on top of ncurses. Maps glyph codes to Unicode (or ASCII) and
/// styles to curses colour pairs.
class CursesTerminal final : public Terminal {
public:
	CursesTerminal() = default;
	CursesTerminal(const CursesTerminal &) = delete;
	CursesTerminal &operator=(const CursesTerminal &) = delete;
	~CursesTerminal() override { close(); }

	/// Starts curses and resizes the terminal to `rows` x `cols`.
	std::expected<void, std::string> open(int rows, int cols);
	/// Ends curses. Safe to call when not open.
	void close();
	bool is_open() const { return open_; }
	/// Whether the terminal shows colours. Valid after open().
	bool has_color() const;

	void draw(int row, int col, const Cell &cell) override;
	void set_cursor(int row, int col) override;
	void show_cursor(bool visible) override;
	void flush() override;
	void bell() override;
	int read_key(int timeout_ms) override;

private:
	bool open_ = false;
	// Terminal size we *want*, not necessarily what we will get
	int want_lines_ = 0;
	int want_cols_ = 0;
	/*
	 * Number of colors we're working with, regardless if terminal has more
	 * colors available. Set by open():
	 * -  0 for monochrome
	 * -  8 for 8 basic colors (light versions will use BOLD text attribute)
	 * - 16 if all 16 PC colors are directly indexable
	 */
	int colors_ = 0;
	// if user allows us to redefine color palette to match original RGB
	bool change_colors_ = true;
	// if user wants to use default terminal foreground / background color
	bool use_terminal_fgbg_ = true;
	int key_mask_ = ~0;  // all bits until open() defines the keys
};

} // namespace rogue::ui
