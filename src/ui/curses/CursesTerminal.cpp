/*
 *  The ncurses backend of rogue::ui::Screen: it maps glyph codes to
 *  Unicode (or ASCII) and styles to curses colour pairs, and reads keys.
 *  It is the only file that includes the system <curses.h>.
 */

#include "ui/curses/CursesTerminal.hpp"

#include <array>
#include <cstdlib>
#include <format>
#include <string>
#include <utility>

#include "core/Config.hpp"
#include "ui/ScreenDisplay.hpp"
#include "glyphs.h"

#define NCURSES_WIDECHAR 1
#include <curses.h>

namespace rogue::ui {

namespace {

// Terminal size we *want*, not necessarily what we will get
int want_lines = MAXLINES;
int want_cols = MAXCOLS;

// ASCII instead of Unicode glyphs (the ROGUE_ASCII build switch)
constexpr bool ascii = rogue::config::ascii_glyphs;

/*
 * Number of colors we're working with, regardless if terminal has more colors
 * available. Set by init_colors():
 * -  0 for monochrome
 * -  8 for 8 basic colors (light versions will use BOLD text attribute)
 * - 16 if all 16 PC colors are directly indexable
 */
int colors = 0;

// if user allows us to redefine color palette to match original RGB
bool change_colors = true;

// if user wants to use default terminal foreground / background color
bool use_terminal_fgbg = true;

int key_mask = ~0;  // all bits until define_keys() knows better

/*
 * How each glyph code is shown. Codes not listed are printable ASCII shown
 * as themselves; anything else shows as '`' ("something went wrong!").
 *
 * Changes in ASCII chars from Unix Rogue (and roguelike ASCII tradition):
 * AMULET: ',' to '&'. Not meant to be subtle in DOS
 * BMAGIC: '+' to '~'. '+' is ASCII for door. BMAGIC is a DOS-only extension.
 *
 * Line cells (boxes, the curtain) use their own ASCII so that frames look
 * different from room walls.
 */
struct GlyphLook {
	std::uint8_t code;
	char ascii;
	char line_ascii;
	wchar_t unicode;
};

constexpr GlyphLook looks[] = {
	// Dungeon
	{PLAYER,  '@', '@', L'\x263A'}, // ☺
	{TRAP,    '^', '^', L'\x2666'}, // ♦
	{FOOD,    ':', ':', L'\x2663'}, // ♣
	{ARMOR,   ']', ']', L'\x25D8'}, // ◘
	{RING,    '=', '=', L'\x25CB'}, // ○
	{AMULET,  '&', '&', L'\x2640'}, // ♀
	{SCROLL,  '?', '?', L'\x266A'}, // ♪
	{GOLD,    '*', '*', L'\x263C'}, // ☼
	{WEAPON,  ')', ')', L'\x2191'}, // ↑ (also "up" on the help screen)
	{POTION,  '!', '!', L'\x00A1'}, // ¡
	{PASSAGE, '#', '#', L'\x2592'}, // ▒ (also the curtain)
	{DOOR,    '+', '+', L'\x256C'}, // ╬
	{STICK,   '/', '/', L'\x03C4'}, // τ
	{FLOOR,   '.', '.', L'\x00B7'}, // ·
	{STAIRS,  '%', '%', L'\x2261'}, // ≡

	// Walls, which double as the double-line box
	{VWALL,   '|', 'H', L'\x2551'}, // ║
	{HWALL,   '-', '=', L'\x2550'}, // ═
	{ULWALL,  '-', '#', L'\x2554'}, // ╔
	{URWALL,  '-', '#', L'\x2557'}, // ╗
	{LLWALL,  '-', '#', L'\x255A'}, // ╚
	{LRWALL,  '-', '#', L'\x255D'}, // ╝

	// Single-line box
	{VLINE,    '|', '|', L'\x2502'}, // │
	{HLINE,    '-', '-', L'\x2500'}, // ─
	{ULCORNER, '.', '.', L'\x250C'}, // ┌
	{URCORNER, '.', '.', L'\x2510'}, // ┐
	{LLCORNER, '`', '`', L'\x2514'}, // └
	{LRCORNER, '/', '\'', L'\x2518'}, // ┘ ('Enter' char 2 as text)

	// Title screen
	{DVLEFT,  'X', 'X', L'\x2563'}, // ╣
	{DVRIGHT, 'X', 'X', L'\x2560'}, // ╠

	// Help screens
	{0x11, '<', '<', L'\x25C4'}, // ◄ 'Enter' char 1
	{0x19, 'v', 'v', L'\x2193'}, // ↓ down
	{0x1A, '>', '>', L'\x2192'}, // → right
	{0x1B, '<', '<', L'\x2190'}, // ← left
	{0xB2, '#', '#', L'\x2593'}, // ▓ 'passage', different char
};

/// looks[] indexed by glyph code
const std::array<const GlyphLook *, 256> &look_table()
{
	static const std::array<const GlyphLook *, 256> table = [] {
		std::array<const GlyphLook *, 256> t{};
		for (const GlyphLook &look : looks)
			t[look.code] = &look;
		return t;
	}();
	return table;
}

wchar_t shown_as(const Cell &cell)
{
	if (cell.ch >= 0x20 && cell.ch < 0x7f)
		return cell.ch;
	const GlyphLook *look = look_table()[cell.ch];
	if (look == nullptr)
		return L'`';
	if (!ascii)
		return look->unicode;
	return cell.line ? look->line_ascii : look->ascii;
}

/*
 * Numpad keys missing from the terminfo data of some common terminals
 * See define_keys()
 *
 * CSI: Control Sequence Introducer: ESC [
 * SS3: Single Shift Select of G3 Character Set (SS3  is 0x8f): ESC O
 *      This affects next character only.
 *
 * xterm
 * *	SS3 j	* multiply
 * +	SS3 k	+ add
 * -	SS3 m	- minus
 * .	SS3 n	. period (VT220)
 * /	SS3 o	/ divide   ()
 * ENT	0527	KEY_ENTER
 * 7	0406	KEY_HOME
 * 5	0536	KEY_B2
 * 1	0550	KEY_END
 *
 * gnome-terminal TERM=xterm
 * .	.
 * 7	CSI 1 ~	Home (VT220)
 * 5	CSI E	5 begin (kb2/K2)
 * 1	CSI 4 ~	End (VT220)
 *
 * gnome-terminal TERM=gnome
 * ENT	SS3 M	CR, enter (kent/@8)
 * 7	0552	KEY_FIND
 * 1	0601	KEY_SELECT
 */
const struct {
	const char *def;
	int dest;
} ttymap[] = {
	{"\033Oj", '*'},
	{"\033Ok", '+'},
	{"\033Om", '-'},
	{"\033On", '.'},
	{"\033Oo", '/'},
	{"\033OM",  KEY_ENTER},
	{"\033[E",  KEY_B2},
	{"\033[1~", KEY_HOME},
	{"\033[4~", KEY_END},
};

/*
 * Curses key codes the game understands, as rogue::ui::key values
 */
const struct {
	int keycode;
	int key;
} keytab[] = {
	{KEY_ENTER,	key::Enter}, // Keypad Enter
	{KEY_HOME,	key::Home},
	{KEY_FIND,	key::Home},  // Keypad Home (7) in some terminals
	{KEY_A1,	key::Home},  // Keypad upper left (7)
	{KEY_UP,	key::Up},
	{KEY_PPAGE,	key::PageUp},  // Page Up
	{KEY_A3,	key::PageUp},  // Keypad upper right (9)
	{KEY_BACKSPACE, key::Backspace},
	{KEY_LEFT,	key::Left},
	{KEY_RIGHT,	key::Right},
	{KEY_END,	key::End},
	{KEY_SELECT,	key::End},  // Keypad End (1) in some terminals
	{KEY_C1,	key::End},  // Keypad lower left (1)
	{KEY_DOWN,	key::Down},
	{KEY_NPAGE,	key::PageDown},  // Page Down
	{KEY_C3,	key::PageDown},  // Keypad lower right (3)
	{KEY_IC,	key::Insert},  // Insert
	{KEY_DC,	key::Delete},  // Delete
	{KEY_F(1),	key::function(1)},
	{KEY_F(2),	key::function(2)},
	{KEY_F(3),	key::function(3)},
	{KEY_F(4),	key::function(4)},
	{KEY_F(5),	key::function(5)},
	{KEY_F(6),	key::function(6)},
	{KEY_F(7),	key::function(7)},
	{KEY_F(8),	key::function(8)},
	{KEY_F(9),	key::function(9)},
	{KEY_F(57),	key::AltF9}  // ALT+F9
};

void define_keys()
{
	// get the shift offset of the bit past KEY_MAX
	int shift = 1;
	for (int i = KEY_MAX; i >>= 1; shift++)
		;

	// define the mask that will be used by read_key()
	key_mask = (1 << shift) - 1;

	// define keys. first key gets i>0 to leave room for user terminfo keys
	int i = 8;
	for (const auto &seq : ttymap)
		if (!key_defined(seq.def))
			define_key(seq.def, ((i++) << shift) | seq.dest);
}

/// ANSI colour index of a PC colour: CGA has red and blue swapped
short ansi(int pc)
{
	return static_cast<short>(((pc & 1) << 2) | (pc & 2) | ((pc & 4) >> 2));
}

short pair_index(int fg, int bg)
{
	return static_cast<short>(bg * colors + fg + 1);
}

/*
 * Original CGA colors
 * https://en.wikipedia.org/wiki/Color_Graphics_Adapter#Color_palette
 * red   := 2/3 * (colorNumber & 4)/4 + 1/3 * (colorNumber & 8)/8
 * green := 2/3 * (colorNumber & 2)/2 + 1/3 * (colorNumber & 8)/8
 * blue  := 2/3 * (colorNumber & 1)/1 + 1/3 * (colorNumber & 8)/8
 * if colorNumber = 6 then green := green / 2
 *
 * These take an ANSI index (red and blue swapped, so brown is 3) and
 * return a value in [0, 1].
 */
double cga_component(int c, int bit)
{
	return !!(c & bit) * 2 / 3.0 + !!(c & 8) * 1 / 3.0;
}
double cga_red(int c) { return cga_component(c, 1); }
double cga_green(int c) { return cga_component(c, 2) / (c == 3 ? 2 : 1); }
double cga_blue(int c) { return cga_component(c, 4); }

/*
 * Some notes on colors and mappings:
 *
 * The PC only has 8 background colours, and the foreground can be bumped
 * to 16 via the bright bit. So background and foreground colour indexes
 * range from 0-15, at most 16 * 16 = 256 colour pairs are needed, always
 * accessed via pair_index(fg, bg).
 *
 * The actual colors mapped to each of these 16 indexes depend on terminal
 * color capabilities:
 * - If only 8, achieve the 16 via curses A_BOLD attribute.
 * - For 16 color terminals there's a 1:1 mapping
 * - For 8 and 16, try to redefine terminal RGB values to match CGA
 * - 88 and 256, remap the 16 color indexes to the 4x4x4 or 6x6x6 color
 *   cube to get an exact CGA color match.
 *
 * Color pair 0 is not initialized and used for plain text. The plain
 * light grey on black maps to the terminal's default colours when the
 * user allows it.
 */
void init_colors()
{
	int cube = 0;
	int cmap[16];
	bool colors_changed = false;

	if (!has_colors() || COLORS < 8) {
		colors = 0;
		return;
	}

	colors = COLORS >= 16 ? 16 : 8;
	if (COLORS >= 256)
		cube = 6;
	else if (COLORS >= 88)
		cube = 4;
	else if (can_change_color() && change_colors)
		colors_changed = true;

	for (int i = 0; i < colors; i++) {
		if (cube) {
			int r = static_cast<int>((cube - 1) * cga_red(i));
			int g = static_cast<int>((cube - 1) * cga_green(i));
			int b = static_cast<int>((cube - 1) * cga_blue(i));
			cmap[i] = 16 + cube * cube * r + cube * g + b;
		} else {
			if (colors_changed)
				init_color(static_cast<short>(i),
				           static_cast<short>(1000 * cga_red(i)),
				           static_cast<short>(1000 * cga_green(i)),
				           static_cast<short>(1000 * cga_blue(i)));
			cmap[i] = i;  // 1:1 mapping
		}
	}

	int plain_fg = ansi(static_cast<int>(Plain.fg));
	int plain_bg = ansi(static_cast<int>(Plain.bg));
	int dfg = cmap[COLOR_WHITE];
	int dbg = cmap[COLOR_BLACK];
	if (use_terminal_fgbg) {
		use_default_colors();
		dfg = dbg = -1;
	}

	for (int bg = 0; bg < colors; bg++)
		for (int fg = colors - (bg ? 2 : 1); fg >= 0; fg--)
			init_pair(pair_index(fg, bg),
			          static_cast<short>(fg == plain_fg ? dfg : cmap[fg]),
			          static_cast<short>(bg == plain_bg ? dbg : cmap[bg]));
}

/// Curses attributes and colour pair for a style
void render(const Style &style, attr_t &attrs, short &pair)
{
	attrs = WA_NORMAL;
	pair = 0;

	// shortcut to avoid setting (and calculating) a spurious color pair
	if (style == Plain)
		return;

	if (style.blink)
		attrs |= WA_BLINK;
	if (style.underline)
		attrs |= WA_UNDERLINE;

	int pc_fg = static_cast<int>(style.fg);
	short fg = ansi(pc_fg & 7);
	short bg = ansi(static_cast<int>(style.bg) & 7);

	if (pc_fg & 8) {
		if (colors < 16)
			attrs |= WA_BOLD;
		else
			fg = static_cast<short>(fg + 8);
	}

	/*
	 * Set terminal reverse attribute when Rogue implies it
	 *
	 * CGA does not have a "reverse" mode, Rogue achieves it by painting a
	 * light grey background (with black or another foreground). By
	 * activating the terminal reverse mode and swapping fg with bg to
	 * revert Rogue's reversal, the terminal's own default colours are used
	 * instead of hard-coded black on white, keeping standout text
	 * consistent with plain text in any terminal colour theme.
	 *
	 * This does not work with 8-color terminals: on reversed mode, A_BOLD
	 * operates on the background color, making it impossible to get yellow
	 * as foreground.
	 */
	if (style.bg == Color::LightGrey && colors != 8 && use_terminal_fgbg) {
		attrs |= WA_REVERSE;
		std::swap(fg, bg);
	}

	if (colors > 0)
		pair = pair_index(fg, bg);
}

void resize_screen()
{
	if ((LINES != want_lines) || (COLS != want_cols))
		if (resizeterm(want_lines, want_cols) == OK)
			flushinp();  // eat up the generated KEY_RESIZE
}

} // namespace

std::expected<void, std::string>
CursesTerminal::open(int rows, int cols)
{
	if (open_)
		return {};

	want_lines = rows;
	want_cols = cols;

	setenv("ESCDELAY", "25", 0);
	initscr();
	open_ = true;
	if ((LINES < want_lines) || (COLS < want_cols)) {
		std::string error = std::format(
				"{}-column mode requires at least a {} x {} screen\n"
				"Your terminal size is {} x {}\n",
				want_cols, want_cols, want_lines, COLS, LINES);
		close();
		return std::unexpected(std::move(error));
	}
	start_color();
	cbreak();  // do not buffer input until ENTER
	noecho();  // do not echo typed characters
	nodelay(stdscr, FALSE); // use a blocking getch() (already the default)
	keypad(stdscr, TRUE);   // enable directional arrows, keypad, home, etc

	resize_screen();
	define_keys();
	init_colors();
	return {};
}

/*
 * Curses resets color RGB based on terminfo, which is somewhat useless, as
 * (1) few terminals have terminfo default colors entries (linux does, xterm
 * does not), and (2) those terminfo colors might not be the current ones
 * before game start. So nothing is restored: the user told us to
 * change_colors, after all.
 */
void
CursesTerminal::close()
{
	if (!open_)
		return;
	endwin();
	open_ = false;
}

bool
CursesTerminal::has_color() const
{
	return colors > 0;
}

/*
 * Paint one cell. mvwadd_wchnstr() neither advances the cursor nor wraps,
 * so the bottom-right corner can be written like any other cell.
 */
void
CursesTerminal::draw(int row, int col, const Cell &cell)
{
	wchar_t text[2] = {shown_as(cell), L'\0'};
	attr_t attrs;
	short pair;
	cchar_t cch;

	render(cell.style, attrs, pair);
	setcchar(&cch, text, attrs, pair, nullptr);
	mvwadd_wchnstr(stdscr, row, col, &cch, 1);
}

void
CursesTerminal::set_cursor(int row, int col)
{
	wmove(stdscr, row, col);
}

void
CursesTerminal::show_cursor(bool visible)
{
	curs_set(visible ? 1 : 0);
}

void
CursesTerminal::flush()
{
	wrefresh(stdscr);
}

void
CursesTerminal::bell()
{
	beep();
}

/*
 * Read a key, waiting at most timeout_ms milliseconds (forever if negative).
 * wget_wch() refreshes the screen first.
 *
 * Return key::None on timeout, non-ASCII chars, window resize and keys the
 * game does not use.
 */
int
CursesTerminal::read_key(int timeout_ms)
{
	wint_t wch;
	int ret;
	int ch;

	wtimeout(stdscr, timeout_ms);
	ret = wget_wch(stdscr, &wch);
	nodelay(stdscr, FALSE);

	// we're only interested in ASCII input and special keys
	if (ret == ERR || (ret == OK && wch > 0x7f))
		return key::None;

	// mask-map custom keys
	ch = key_mask & static_cast<int>(wch);

	// window resize needs special handling
	if (ch == KEY_RESIZE) {
		resize_screen();
		return key::None;
	}
	if (ch < KEY_MIN)
		return ch;
	for (const auto &k : keytab)
		if (ch == k.keycode)
			return k.key;
	return key::None;
}

// The terminal the game runs on

namespace {

CursesTerminal the_terminal;

} // namespace

std::expected<void, std::string> start_terminal(bool monochrome)
{
	if (the_terminal.is_open())
		return {};
	if (auto opened = the_terminal.open(Screen::Rows, Screen::Cols); !opened)
		return opened;
	screen_display().set_monochrome(monochrome || !the_terminal.has_color());
	screen().connect(&the_terminal);
	return {};
}

void stop_terminal()
{
	screen().connect(nullptr);
	the_terminal.close();
}

} // namespace rogue::ui
