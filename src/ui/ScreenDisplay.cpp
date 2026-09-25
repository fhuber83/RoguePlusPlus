#include "ui/ScreenDisplay.hpp"

#include <chrono>
#include <cstdio>
#include <thread>

#include "glyphs.h"

namespace rogue::ui {

namespace {

constexpr int StatusRow = 23;
constexpr int HungerRow = 24;
constexpr int ClockCol = 75;

/// Total time, in milliseconds, of each curtain animation
constexpr int CurtainTime = 1500;

constexpr Style Bright{Color::White};


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
	ink(Ink::Reverse);
	text(prompt_);
	ink(Ink::Normal);
	prompt_shown_ = true;
}

void ScreenDisplay::draw_covered()
{
	screen_.set_cursor(0, prompt_col_);
	for (std::size_t i = 0; i < prompt_.size(); i++)
		screen_.put(covered_cells_[i].ch, covered_cells_[i].style);
	prompt_shown_ = false;
}

// Map

namespace {

Ink ink_of(TileStyle style)
{
	switch (style) {
	case TileStyle::Normal: return Ink::Normal;
	case TileStyle::Inverse: return Ink::Reverse;
	case TileStyle::Bolt: return Ink::Red;
	case TileStyle::FrostBolt: return Ink::Blue;
	}
	return Ink::Normal;
}

} // namespace

void ScreenDisplay::draw_tile(Coord pos, std::uint8_t glyph, TileStyle style)
{
	if (!screen_.set_cursor(pos.y, pos.x))
		return;
	screen_.put(glyph, glyph_style(glyph, style_for(ink_of(style))));
}

void ScreenDisplay::animation_pause(int ms) const
{
	if (animations_)
		std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

std::uint8_t ScreenDisplay::tile_at(Coord pos) const
{
	return Screen::contains(pos.y, pos.x) ? screen_.at(pos.y, pos.x).ch : ' ';
}

TileStyle ScreenDisplay::tile_style_at(Coord pos) const
{
	if (!Screen::contains(pos.y, pos.x))
		return TileStyle::Normal;
	const Cell &cell = screen_.at(pos.y, pos.x);
	for (TileStyle style : {TileStyle::Normal, TileStyle::Inverse, TileStyle::Bolt, TileStyle::FrostBolt})
		if (cell.style == glyph_style(cell.ch, style_for(ink_of(style))))
			return style;
	return TileStyle::Normal;
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

	ink(Ink::Yellow);

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
			ink(Ink::Bold);
			text(hunger_names[s.hunger]);
			ink(Ink::Normal);
		}
	}

	ink(Ink::Normal);
	screen_.set_cursor(row, col);
}

void ScreenDisplay::draw_clock(int hour, int minute)
{
	char buf[8];
	int row = screen_.row(), col = screen_.col();

	std::snprintf(buf, sizeof buf, "%2d:%02d", hour, minute);
	ink(Ink::Bold);
	text_at(HungerRow, ClockCol, buf);
	ink(Ink::Normal);
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
	Style old = screen_.style();
	screen_.set_style(style_for(ink));
	text(s);
	screen_.set_style(old);
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
	ink(Ink::Brown);
	frame(0, 0, Screen::Rows - 1, Screen::Cols - 1, false);
	ink(Ink::Bold);
	centered(2, "ROGUE:  The Adventure Game");
	Ink subtitle = Ink::LightMagenta;
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
	ink(Ink::Yellow);
	centered(19, "(C)Copyright 1985");
	ink(Ink::Bright);
	centered(20, "Epyx Incorporated");
	ink(Ink::Yellow);
	centered(21, "All Rights Reserved");
	ink(Ink::Brown);
	screen_.set_cursor(22, 0);
	glyph(DVRIGHT);
	screen_.line(22, 1, DHLINE, Screen::Cols - 2, false);
	screen_.set_cursor(22, Screen::Cols - 1);
	glyph(DVLEFT);
	ink(Ink::Normal);
	text_at(23, 2, "Rogue's Name? ");
	ink(Ink::Bright);
}

void ScreenDisplay::end_title()
{
	clear_line(23);
	clear_line(24);
	ink(Ink::Brown);
	screen_.set_cursor(22, 0);
	glyph(LLWALL);
	screen_.set_cursor(22, Screen::Cols - 1);
	glyph(LRWALL);
	ink(Ink::Normal);
}

/*@
 * Originally in death(), rip.c
 */
void ScreenDisplay::draw_tombstone(std::string_view name, std::string_view killer, int gold, int year)
{
	char buf[40];

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
	text_at(0, 0, "Guildmaster's Hall Of Fame:");
	ink(Ink::Yellow);
	text_at(2, 0, "Gold");

	for (int i = 0; i < static_cast<int>(lines.size()); i++) {
		const ScoreLine &line = lines[i];
		bool mine = (i == highlight);
		ink(mine ? Ink::Yellow : Ink::Brown);
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
	int delay = CurtainTime / Screen::Rows;

	screen_.show_cursor(false);
	ink(Ink::Green);
	frame(0, 0, Screen::Rows - 1, Screen::Cols - 1, true);
	screen_.refresh();
	animation_pause(delay);  // not in original
	ink(Ink::Yellow);
	for (int r = 1; r < Screen::Rows - 1; r++) {
		screen_.line(r, 1, PASSAGE, Screen::Cols - 2, false);
		screen_.refresh();
		animation_pause(delay);
	}
	curtain_ = screen_.snapshot();
	animation_pause(delay);  // not in original, optional
	screen_.set_cursor(0, 0);
	ink(Ink::Normal);
	screen_.erase();
}

/*@
 * Display a curtain up animation over what was drawn behind the curtain
 */
void ScreenDisplay::curtain_up()
{
	int delay = CurtainTime / Screen::Rows;
	Screen::Snapshot shown = screen_.snapshot();

	screen_.restore(curtain_);
	for (int line = Screen::Rows - 1; line >= 0; line--) {
		screen_.restore_row(shown, line);
		screen_.refresh();
		animation_pause(delay);
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
		animation_pause(delay);
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

/*@
 * The styles of the original's text attribute table (set_attr() indexes),
 * one table for colour screens and one for monochrome ones
 */
Style ScreenDisplay::style_for(Ink ink) const
{
	if (monochrome_) {
		switch (ink) {
		case Ink::Underline: return Style{Color::LightGrey, Color::Black, false, true};
		case Ink::Reverse:
		case Ink::Bold: return Style{Color::DarkGrey, Color::LightGrey};
		default: return Plain;
		}
	}
	switch (ink) {
	case Ink::Normal: return Plain;
	case Ink::Reverse: return Standout;
	case Ink::Bold: return Standout;
	case Ink::Bright: return Bright;
	case Ink::Underline: return Bright;
	case Ink::Red: return Style{Color::Red};
	case Ink::Green: return Style{Color::Green};
	case Ink::Brown: return Style{Color::Brown};
	case Ink::Yellow: return Style{Color::Yellow};
	case Ink::Blue: return Style{Color::Blue};
	case Ink::LightMagenta: return Style{Color::LightMagenta};
	}
	return Plain;
}

/*@
 * The colour a glyph gets on a colour screen: map glyphs have their own
 * colours in rooms (plain text) and in passages (standout). Moved here from
 * cur_addch() in curses.c
 */
Style ScreenDisplay::glyph_style(std::uint8_t ch, Style base) const
{
	if (monochrome_)
		return base;
	if (base == Plain) {
		switch (ch) {
		case DOOR:
		case VWALL:
		case HWALL:
		case ULWALL:
		case URWALL:
		case LLWALL:
		case LRWALL:
			return Style{Color::Brown};
		case FLOOR:
			return Style{Color::LightGreen};
		case STAIRS:
			return Style{Color::Black, Color::Green, true};
		case TRAP:
			return Style{Color::Magenta};
		case GOLD:
		case PLAYER:
			return Style{Color::Yellow};
		case POTION:
		case SCROLL:
		case STICK:
		case ARMOR:
		case AMULET:
		case RING:
		case WEAPON:
			return Style{Color::LightBlue};
		case FOOD:
			return Style{Color::Red};
		}
	} else if (base == Standout) {
		switch (ch) {
		case FOOD:
			return Style{Color::Red, Color::LightGrey};
		case GOLD:
		case PLAYER:
			return Style{Color::Yellow, Color::LightGrey};
		case POTION:
		case SCROLL:
		case STICK:
		case ARMOR:
		case AMULET:
		case RING:
		case WEAPON:
			return Style{Color::Blue, Color::LightGrey};
		}
	} else if (base == Bright && ch == STAIRS) {
		//@ I suspect STAIRS used with high() is a case that never happen...
		return Style{Color::Black, Color::Green, true};
	}
	return base;
}

void ScreenDisplay::ink(Ink ink)
{
	screen_.set_style(style_for(ink));
}

/// Writes one glyph at the cursor
void ScreenDisplay::glyph(std::uint8_t ch)
{
	screen_.put(ch, glyph_style(ch, screen_.style()));
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

/// Writes at the cursor, colouring glyphs the way cur_addch() did.
void ScreenDisplay::text(std::string_view s)
{
	for (char c : s)
		glyph(static_cast<std::uint8_t>(c));
}

void ScreenDisplay::text_at(int row, int col, std::string_view s)
{
	screen_.set_cursor(row, col);
	text(s);
}

ScreenDisplay &screen_display()
{
	static ScreenDisplay instance(screen());
	return instance;
}

Display &display()
{
	return screen_display();
}

} // namespace rogue::ui
