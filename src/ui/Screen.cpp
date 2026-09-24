#include "ui/Screen.hpp"

#include "ui/Terminal.hpp"

namespace rogue::ui {

void Screen::connect(Terminal *terminal)
{
	terminal_ = terminal;
	if (terminal_ == nullptr)
		return;
	for (int r = 0; r < Rows; r++)
		for (int c = 0; c < Cols; c++)
			draw(r, c);
	terminal_->show_cursor(cursor_visible_);
	terminal_->set_cursor(row_, col_);
}

bool Screen::set_cursor(int row, int col)
{
	if (!contains(row, col))
		return false;
	row_ = row;
	col_ = col;
	return true;
}

bool Screen::show_cursor(bool visible)
{
	bool was = cursor_visible_;
	cursor_visible_ = visible;
	if (terminal_)
		terminal_->show_cursor(visible);
	return was;
}

void Screen::put(std::uint8_t ch, Style style)
{
	if (ch == '\n') {
		erase_to_eol();
		if (row_ < Rows - 1) {
			row_++;
			col_ = 0;
		}
		return;
	}
	cells_[row_][col_] = Cell{ch, style, false};
	draw(row_, col_);
	if (col_ < Cols - 1)
		col_++;
	else if (row_ < Rows - 1) {
		row_++;
		col_ = 0;
	}
}

void Screen::put(std::string_view text)
{
	for (char ch : text)
		put(static_cast<std::uint8_t>(ch));
}

void Screen::line(int row, int col, std::uint8_t ch, int length, bool vertical)
{
	for (int i = 0; i < length; i++) {
		int r = vertical ? row + i : row;
		int c = vertical ? col : col + i;
		if (!contains(r, c))
			break;
		set(r, c, Cell{ch, style_, true});
	}
}

void Screen::set(int row, int col, const Cell &cell)
{
	cells_[row][col] = cell;
	draw(row, col);
}

void Screen::erase()
{
	for (int r = 0; r < Rows; r++)
		for (int c = 0; c < Cols; c++)
			set(r, c, Cell{});
	row_ = col_ = 0;
}

void Screen::erase_to_eol()
{
	for (int c = col_; c < Cols; c++)
		set(row_, c, Cell{});
}

void Screen::restore(const Snapshot &shot)
{
	for (int r = 0; r < Rows; r++)
		restore_row(shot, r);
}

void Screen::restore_row(const Snapshot &shot, int row)
{
	for (int c = 0; c < Cols; c++)
		set(row, c, shot[row][c]);
}

void Screen::refresh()
{
	if (terminal_) {
		terminal_->set_cursor(row_, col_);
		terminal_->flush();
	}
}

void Screen::bell()
{
	if (terminal_)
		terminal_->bell();
}

int Screen::read_key(int timeout_ms)
{
	if (terminal_ == nullptr)
		return key::None;
	terminal_->set_cursor(row_, col_);
	return terminal_->read_key(timeout_ms);
}

void Screen::draw(int row, int col)
{
	if (terminal_)
		terminal_->draw(row, col, cells_[row][col]);
}

Screen &screen()
{
	static Screen instance;
	return instance;
}

} // namespace rogue::ui
