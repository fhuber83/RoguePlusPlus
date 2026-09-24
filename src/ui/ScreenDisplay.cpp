#include "ui/ScreenDisplay.hpp"

#include <cstdio>

#include "curses_common.h"

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

/// set_attr() index for each Ink
int ink_index(Ink ink)
{
	switch (ink) {
	case Ink::Normal: return 0;
	case Ink::Green: return 1;
	case Ink::Red: return 3;
	case Ink::Brown: return 5;
	case Ink::LightMagenta: return 10;
	case Ink::Yellow: return 11;
	case Ink::Underline: return 12;
	case Ink::Blue: return 13;
	case Ink::Reverse: return 14;
	case Ink::Bright: return 15;
	case Ink::Bold: return 16;
	}
	return 0;
}

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

// Pages

void ScreenDisplay::open_page()
{
	game_view_ = screen_.snapshot();
	page_open_ = true;
}

void ScreenDisplay::close_page()
{
	screen_.restore(game_view_);
	screen_.refresh();
	page_open_ = false;
}

void ScreenDisplay::clear_page()
{
	screen_.erase();
}

// Text

Coord ScreenDisplay::write_at(int row, int col, std::string_view s, Ink ink)
{
	screen_.set_cursor(row, col);
	return write(s, ink);
}

Coord ScreenDisplay::write(std::string_view s, Ink ink)
{
	std::uint8_t old = screen_.attr();
	screen_.set_attr(dos_attr(ink_index(ink)));
	text(s);
	screen_.set_attr(old);
	return Coord{screen_.col(), screen_.row()};
}

void ScreenDisplay::clear_line(int row, int col)
{
	screen_.set_cursor(row, col);
	screen_.erase_to_eol();
}

bool ScreenDisplay::show_cursor(bool visible)
{
	return screen_.show_cursor(visible);
}

// Output

// Title and ending screens

/*@
 * The credits screen, originally credits() in mach_dep.c
 */
void ScreenDisplay::draw_title()
{
	screen_.show_cursor(false);
	screen_.erase();
	if (is_color)
		ink(Ink::Brown);
	frame(0, 0, Screen::Rows - 1, Screen::Cols - 1, false);
	ink(Ink::Bold);
	centered(2, "ROGUE:  The Adventure Game");
	Ink subtitle = is_color ? Ink::LightMagenta : Ink::Underline;
	ink(subtitle);
	centered(4, "The game of Rogue was designed by:");
	ink(Ink::Bright);
	centered(6, "Michael Toy and Glenn Wichman");
	ink(subtitle);
	centered(9, "Various implementations by:");
	ink(Ink::Bright);
	centered(11, "Ken Arnold, Jon Lane and Michael Toy");
	ink(subtitle);
	centered(14, "Adapted for the IBM PC by:");
	ink(Ink::Bright);
	centered(16, "A.I. Design");
	ink(subtitle);
	if (is_color)
		ink(Ink::Yellow);
	centered(19, "(C)Copyright 1985");
	ink(Ink::Bright);
	centered(20, "Epyx Incorporated");
	ink(Ink::Normal);
	if (is_color)
		ink(Ink::Yellow);
	centered(21, "All Rights Reserved");
	if (is_color)
		ink(Ink::Brown);
	screen_.set_cursor(22, 0);
	text("\xcc"); // DVRIGHT
	screen_.line(22, 1, DHLINE, Screen::Cols - 2, false);
	screen_.set_cursor(22, Screen::Cols - 1);
	text("\xb9"); // DVLEFT
	ink(Ink::Normal);
	text_at(23, 2, "Rogue's Name? ");
	ink(Ink::Bright);
}

void ScreenDisplay::end_title()
{
	clear_line(23);
	clear_line(24);
	if (is_color)
		ink(Ink::Brown);
	text_at(22, 0, "\xc8");               // LLWALL
	text_at(22, Screen::Cols - 1, "\xbc"); // LRWALL
	ink(Ink::Normal);
}

/*@
 * Originally in death(), rip.c
 */
void ScreenDisplay::draw_tombstone(std::string_view name, std::string_view killer, int gold, int year)
{
	char buf[40];

	if (is_color)
		ink(Ink::Brown);
	frame(7, (Screen::Cols - 28) / 2, 22, (Screen::Cols + 28) / 2, false);
	ink(Ink::Normal);

	centered(10, "REST");
	centered(11, "IN");
	centered(12, "PEACE");
	ink(Ink::Red);
	centered(21, "  *    *      * ");
	ink(Ink::Green);
	centered(22, "___\\/(\\/)/(\\/ \\\\(//)\\)\\/(//)\\\\)//(\\__");
	ink(Ink::Normal);

	if (scr_type == 7)
		ink(Ink::Underline);
	centered(14, name);
	ink(Ink::Normal);

	centered(15, "killed by");
	centered(16, killer);
	std::snprintf(buf, sizeof buf, "%u Au", static_cast<unsigned>(gold));
	centered(18, buf);
	std::snprintf(buf, sizeof buf, "%u", static_cast<unsigned>(year));
	centered(19, buf);
}

/*@
 * Originally pr_scores() in rip.c
 */
void ScreenDisplay::draw_scores(std::span<const ScoreLine> lines, int highlight)
{
	char buf[40];

	screen_.erase();
	ink(Ink::Bright);
	if (scr_type == 7)
		ink(Ink::Reverse);
	text_at(0, 0, "Guildmaster's Hall Of Fame:");
	ink(Ink::Normal);
	ink(Ink::Yellow);
	text_at(2, 0, "Gold");

	for (int i = 0; i < static_cast<int>(lines.size()); i++) {
		const ScoreLine &line = lines[i];
		bool mine = (i == highlight);
		Ink own = (scr_type == 7) ? Ink::Reverse : Ink::Yellow;

		ink(mine ? own : Ink::Brown);
		std::snprintf(buf, sizeof buf, "%d ", line.gold);
		text_at(4 + i, 0, buf);
		screen_.set_cursor(4 + i, 6);
		if (!mine)
			ink(Ink::Red);
		text(line.name);
		if (!mine)
			ink(Ink::Brown);
		text(line.text);
	}
	ink(Ink::Normal);
	text("\n\n\n\n");
}

/*@
 * Originally in total_winner(), rip.c
 */
void ScreenDisplay::draw_winner(bool brief)
{
	screen_.erase();
	if (!brief) {
		ink(Ink::Reverse);
		text("                                                               \n");
		text("  @   @               @   @           @          @@@  @     @  \n");
		text("  @   @               @@ @@           @           @   @     @  \n");
		text("  @   @  @@@  @   @   @ @ @  @@@   @@@@  @@@      @  @@@    @  \n");
		text("   @@@@ @   @ @   @   @   @     @ @   @ @   @     @   @     @  \n");
		text("      @ @   @ @   @   @   @  @@@@ @   @ @@@@@     @   @     @  \n");
		text("  @   @ @   @ @  @@   @   @ @   @ @   @ @         @   @  @     \n");
		text("   @@@   @@@   @@ @   @   @  @@@@  @@@@  @@@     @@@   @@   @  \n");
	}
	text("                                                               \n");
	text("     Congratulations, you have made it to the light of day!    \n");
	ink(Ink::Normal);
	text("\nYou have joined the elite ranks of those who have escaped the\n");
	text("Dungeons of Doom alive.  You journey home and sell all your loot at\n");
	text("a great profit and are admitted to the fighters guild.\n");
	text_at(Screen::Rows - 1, 0, "--Press space to continue--");
}

// Transitions

/*@
 * Display a curtain down animation, keep the curtain and blank the screen
 * without showing it. Moved from curses.c
 */
void ScreenDisplay::curtain_down()
{
	int delay = CURTAIN_TIME / Screen::Rows;

	screen_.show_cursor(false);
	ink(Ink::Green);
	frame(0, 0, Screen::Rows - 1, Screen::Cols - 1, true);
	screen_.refresh();
	msleep(delay);  // not in original
	ink(Ink::Yellow);
	for (int r = 1; r < Screen::Rows - 1; r++) {
		screen_.line(r, 1, FILLER, Screen::Cols - 2, false);
		screen_.refresh();
		msleep(delay);
	}
	curtain_ = screen_.snapshot();
	msleep(delay);  // not in original, optional
	screen_.set_cursor(0, 0);
	ink(Ink::Normal);
	screen_.erase();
}

/*@
 * Display a curtain up animation over what was drawn behind the curtain
 */
void ScreenDisplay::curtain_up()
{
	int delay = CURTAIN_TIME / Screen::Rows;
	Screen::Snapshot shown = screen_.snapshot();

	screen_.restore(curtain_);
	for (int line = Screen::Rows - 1; line >= 0; line--) {
		screen_.restore_row(shown, line);
		screen_.refresh();
		msleep(delay);
	}
}

/*
 * Clear the screen in an interesting fashion
 * @ implode(), moved from curses.c
 */
void ScreenDisplay::wipe()
{
	int j, delay, r, c, cinc = Screen::Cols/10/2, er, ec;

	er = Screen::Rows-3;
	delay = 50;
	for (r = 0,c = 0,ec = Screen::Cols-1; r < 10; r++,c += cinc,er--,ec -= cinc) {
		frame(r, c, er, ec, true);
		screen_.refresh();
		msleep(delay);
		for (j = r+1; j <= er-1; j++) {
			screen_.line(j, c+1, ' ', cinc-1, false);
			screen_.line(j, ec-cinc+1, ' ', cinc-1, false);
		}
		for (j = r; j <= er; j++) {
			screen_.line(j, c, ' ', 1, false);
			screen_.line(j, ec, ' ', 1, false);
		}
		screen_.line(r, c, ' ', ec - c + 1, false);
		screen_.line(er, c, ' ', ec - c + 1, false);
	}
	screen_.refresh();
}

void ScreenDisplay::flush()
{
	screen_.refresh();
}

void ScreenDisplay::bell()
{
	screen_.bell();
}

// Helpers

void ScreenDisplay::ink(Ink ink)
{
	screen_.set_attr(dos_attr(ink_index(ink)));
}

/// Centres `s` on `row`, as center() did
void ScreenDisplay::centered(int row, std::string_view s)
{
	text_at(row, (Screen::Cols - static_cast<int>(s.size())) / 2, s);
}

/// Draws a double (or single) line box, as vbox() did
void ScreenDisplay::frame(int top, int left, int bottom, int right, bool single)
{
	const std::uint8_t ul = single ? ULCORNER : DULCORNER, ur = single ? URCORNER : DURCORNER;
	const std::uint8_t ll = single ? LLCORNER : DLLCORNER, lr = single ? LRCORNER : DLRCORNER;
	const std::uint8_t hl = single ? HLINE : DHLINE, vl = single ? VLINE : DVLINE;
	bool was = screen_.show_cursor(false);

	screen_.line(top, left + 1, hl, right - left - 1, false);
	screen_.line(bottom, left + 1, hl, right - left - 1, false);
	screen_.line(top + 1, left, vl, bottom - top - 1, true);
	screen_.line(top + 1, right, vl, bottom - top - 1, true);
	screen_.line(top, left, ul, 1, false);
	screen_.line(top, right, ur, 1, false);
	screen_.line(bottom, left, ll, 1, false);
	screen_.line(bottom, right, lr, 1, false);
	screen_.show_cursor(was);
}

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
