#include "ui/ScreenDisplay.hpp"

#include <cstdio>

#include "curses.h"

namespace rogue::ui {

namespace {

// set_attr() indexes, see the colour macros in curses_common.h
constexpr int Plain = 0;
constexpr int RedText = 3;
constexpr int YellowText = 11;
constexpr int BlueText = 13;
constexpr int Reverse = 14;
constexpr int BoldText = 16;

constexpr int StatusRow = 23;
constexpr int HungerRow = 24;
constexpr int ClockCol = 75;

constexpr const char *hunger_names[] = {"      ", "Hungry", "Weak", "Faint", "?"};

} // namespace

// Message line

void ScreenDisplay::draw_message(std::string_view s)
{
	screen_.set_cursor(0, 0);
	if (s.size() < static_cast<std::size_t>(Screen::Cols))
		screen_.erase_to_eol();
	text(s.substr(0, Screen::Cols));
}

void ScreenDisplay::clear_message()
{
	screen_.set_cursor(0, 0);
	screen_.erase_to_eol();
}

/*@
 * More: tag the end of a line and wait for a space. The waiting is the
 * game's; this only draws. Originally more() in io.c, which read the covered
 * characters back with inch() and rewrote them as plain text.
 */
void ScreenDisplay::show_more(std::string_view prompt, int col)
{
	int size = static_cast<int>(prompt.size());

	prompt_.assign(prompt);
	prompt_col_ = col;
	covering_ = false;
	if (prompt_col_ + size > Screen::Cols) {
		prompt_col_ = Screen::Cols - size;
		covering_ = true;
	}
	for (int i = 0; i < size; i++)
		covered_cells_[i] = screen_.at(0, prompt_col_ + i);
	draw_prompt();
}

void ScreenDisplay::blink_more()
{
	if (!covering_)
		return;
	if (prompt_shown_)
		draw_covered();
	else
		draw_prompt();
}

void ScreenDisplay::hide_more()
{
	draw_covered();
}

void ScreenDisplay::draw_prompt()
{
	screen_.set_cursor(0, prompt_col_);
	screen_.set_attr(dos_attr(Reverse));
	text(prompt_);
	screen_.set_attr(dos_attr(Plain));
	prompt_shown_ = true;
}

void ScreenDisplay::draw_covered()
{
	screen_.set_cursor(0, prompt_col_);
	for (std::size_t i = 0; i < prompt_.size(); i++)
		screen_.put(covered_cells_[i].ch, covered_cells_[i].attr);
	prompt_shown_ = false;
}

// Map

void ScreenDisplay::draw_tile(Coord pos, std::uint8_t glyph, TileStyle style)
{
	int base = Plain;
	switch (style) {
	case TileStyle::Normal: base = Plain; break;
	case TileStyle::Inverse: base = Reverse; break;
	case TileStyle::Bolt: base = RedText; break;
	case TileStyle::FrostBolt: base = BlueText; break;
	}
	if (!screen_.set_cursor(pos.y, pos.x))
		return;
	screen_.put(glyph, glyph_attr(glyph, dos_attr(base)));
}

std::uint8_t ScreenDisplay::tile_at(Coord pos) const
{
	return Screen::contains(pos.y, pos.x) ? screen_.at(pos.y, pos.x).ch : ' ';
}

// Status lines

/*@
 * Rogue used a rudimentary custom sprintf() that didn't fully support
 * the (quite sophisticated) numeric formatting strings used on status.
 * As <stdio.h>'s sprintf() does, formatting was simplified so the output
 * matches the original.
 */
void ScreenDisplay::draw_status(const Status &s)
{
	char buf[40];
	int row = screen_.row(), col = screen_.col();

	if (is_color)
		screen_.set_attr(dos_attr(YellowText));

	if (level_ != s.level) {
		level_ = s.level;
		std::snprintf(buf, sizeof buf, "Level:%-4d", s.level);
		text_at(StatusRow, 0, buf);
	}
	if (hp_ != s.hp || hp_max_ != s.hp_max) {
		hp_ = s.hp;
		hp_max_ = s.hp_max;
		std::snprintf(buf, sizeof buf, "Hits:%d(%d) ", s.hp, s.hp_max);
		text_at(StatusRow, 12, buf);
		/* just in case they get wraithed with 3 digit max hits */
		if (s.hp < 100)
			text(" ");
	}
	if (str_ != s.str || str_max_ != s.str_max) {
		str_ = s.str;
		str_max_ = s.str_max;
		std::snprintf(buf, sizeof buf, "Str:%u(%u) ", s.str, s.str_max);
		text_at(StatusRow, 26, buf);
	}
	if (gold_ != s.gold) {
		gold_ = s.gold;
		std::snprintf(buf, sizeof buf, "Gold:%-5u", static_cast<unsigned>(s.gold));
		text_at(StatusRow, 40, buf);
	}
	if (armor_ != s.armor) {
		armor_ = s.armor;
		std::snprintf(buf, sizeof buf, "Armor:%-2d", s.armor);
		text_at(StatusRow, 52, buf);
	}
	if (rank_ != s.rank) {
		rank_ = std::string(s.rank);
		std::snprintf(buf, sizeof buf, "%-12.*s", static_cast<int>(s.rank.size()), s.rank.data());
		text_at(StatusRow, 62, buf);
	}
	if (hunger_ != s.hunger) {
		hunger_ = s.hunger;
		text_at(HungerRow, 58, hunger_names[0]);
		if (s.hunger) {
			screen_.set_cursor(HungerRow, 58);
			screen_.set_attr(dos_attr(BoldText));
			text(hunger_names[s.hunger]);
			screen_.set_attr(dos_attr(Plain));
		}
	}

	if (is_color)
		screen_.set_attr(dos_attr(Plain));
	screen_.set_cursor(row, col);
}

void ScreenDisplay::draw_clock(int hour, int minute)
{
	char buf[8];
	int row = screen_.row(), col = screen_.col();

	std::snprintf(buf, sizeof buf, "%2d:%02d", hour, minute);
	screen_.set_attr(dos_attr(BoldText));
	text_at(HungerRow, ClockCol, buf);
	screen_.set_attr(dos_attr(Plain));
	screen_.set_cursor(row, col);
}

void ScreenDisplay::draw_count(int count)
{
	char buf[8] = "    ";

	if (count)
		std::snprintf(buf, sizeof buf, "%-4d", count);
	text_at(StatusRow, Screen::Cols - 4, buf);
}

// Output

void ScreenDisplay::flush()
{
	screen_.refresh();
}

void ScreenDisplay::bell()
{
	screen_.bell();
}

// Helpers

/// Writes at the cursor, colouring glyphs the way cur_addch() does.
void ScreenDisplay::text(std::string_view s)
{
	for (char c : s) {
		auto ch = static_cast<std::uint8_t>(c);
		screen_.put(ch, glyph_attr(ch, screen_.attr()));
	}
}

void ScreenDisplay::text_at(int row, int col, std::string_view s)
{
	screen_.set_cursor(row, col);
	text(s);
}

Display &display()
{
	static ScreenDisplay instance(screen());
	return instance;
}

} // namespace rogue::ui
