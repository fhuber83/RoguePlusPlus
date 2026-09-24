#pragma once

#include <expected>
#include <string>

#include "ui/Terminal.hpp"

namespace rogue::ui {

/// Terminal on top of ncurses. Maps CP437 cells to ASCII or Unicode and DOS
/// attributes to curses colour pairs.
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
};

} // namespace rogue::ui
