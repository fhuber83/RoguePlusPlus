/*
 *  Cursor motion stuff to simulate a "no refresh" version of curses
 *
 *  @ Now only the ncurses backend of rogue::ui::Screen: it maps CP437 cells
 *  @ and DOS attributes to curses characters and colour pairs, and reads keys.
 *  @ The DOS screen API the game calls lives in ui/DosScreen.cpp.
 */

#include	"ui/curses/CursesTerminal.hpp"

#include	<format>

#include	"extern.h"



/*@
 * Many references on the 'net suggest including <ncursesw/curses.h> directly,
 * but this is a task for the Makefile/build system, not source code.
 *
 * <curses.h> will set _XOPEN_CURSES if wide chars are available
 * https://pubs.opengroup.org/onlinepubs/7908799/xcurses/curses.h.html
 * https://publications.opengroup.org/c094
 */
#undef _XOPEN_CURSES
#include	<curses.h>

#include	"curses_common.h"
#include	"ui/curses/curses_dos.h"

using rogue::ui::key::None;
namespace key = rogue::ui::key;


// Terminal size we *want*, not necessarily what we will get
static int cur_LINES = MAXLINES;
static int cur_COLS  = MAXCOLS;

/* Charset used. Could be initially set via env file, but should not be changed
 * mid-game unless we create a function to re-draw the screen. The code should
 * already graciously fallback to ASCII if UNICODE is requested here but not
 * available, either because it was not compiled with wide char support or the
 * current curses implementation does not support it. Both cases are tested
 * with _XOPEN_CURSES. ROGUE_CHARSET is a factory default that already accounts
 * for Unicode availability and compile-time options.
 */
static int	charset = ROGUE_CHARSET;

/*
 * Number of colors we're working with, regardless if terminal has more colors
 * available. This is set by init_curses_colors() and should be the result of
 * many factors, not only terminal COLORS reported by curses but also `screen`
 * env file setting (for bw), ROGUE_SCR_TYPE, etc.
 *
 * Values can be:
 * -  0 for monochrome
 * -  8 for 8 basic colors (light versions will use BOLD text attribute)
 * - 16 if all DOS colors are directly indexable
 */
static int 	colors;

// if user allows us to redefine color palette to match original RGB
static bool change_colors = TRUE;

// if user wants to use default terminal foreground / background color
static bool use_terminal_fgbg = TRUE;

//@ private
#ifdef ROGUE_WIDECHAR
static cchar_t  cctemp;
#endif // ROGUE_WIDECHAR
static int	KEY_MASK = ~0;  //@ all bits until define_keys() knows better
static wchar_t	ccunicode[2] = L" ";  // temp buffer
static CCODE	ccode = {'\0', ccunicode, '\0'};  // temp charcode
static bool	colors_changed = FALSE;  // if colors palette was redefined

/*@
 * Changes in ASCII chars from Unix Rogue (and roguelike ASCII tradition):
 * AMULET: ',' to '&'. Not meant to be subtle in DOS
 * BMAGIC: '+' to '~'. '+' is ASCII for door. BMAGIC is a DOS-only extension.
 *
 * Note: Swapping '+' with '{' would yield a better visual result, as '{' is
 * great as door and it better matches DOS CP437 char 0xCE ╬. But I will not be
 * the one to break such a well-known convention, and get flamed for heresy.
 * You do it.
 */
static CCODE ctab[] = {
		/*
		 * Dungeon chars. If a char in this block is not unique, such as
		 * the ASCII for room corners, cur_inch() reverse search will map
		 * them back to a different DOS char. So choose them carefully.
		 */
		{'@', L"\x263A", PLAYER},     // ☺
		{'^', L"\x2666", TRAP},       // ♦
		{':', L"\x2663", FOOD},       // ♣
		{']', L"\x25D8", ARMOR},      // ◘
		{'=', L"\x25CB", RING},       // ○
		{'&', L"\x2640", AMULET},     // ♀
		{'?', L"\x266A", SCROLL},     // ♪
		{'*', L"\x263C", GOLD},       // ☼
		{')', L"\x2191", WEAPON},     // ↑
		{'!', L"\x00A1", POTION},     // ¡
		{'#', L"\x2592", PASSAGE},    // ▒
		{'+', L"\x256C", DOOR},       // ╬
		{'/', L"\x03C4", STICK},      // τ
		{'.', L"\x00B7", FLOOR},      // ·
		{'%', L"\x2261", STAIRS},     // ≡
//		{'$', L"$",      MAGIC},      // $, maps to itself
//		{'~', L"~",      BMAGIC},     // ~, maps to itself
		{'|', L"\x2551", VWALL},      // ║
		{'-', L"\x2550", HWALL},      // ═
		{'-', L"\x2554", ULWALL},     // ╔
		{'-', L"\x2557", URWALL},     // ╗
		{'-', L"\x255A", LLWALL},     // ╚
		{'-', L"\x255D", LRWALL},     // ╝

		// Title screen
		{'X', L"\x2563", DVLEFT},     // ╣
		{'X', L"\x2560", DVRIGHT},    // ╠

		// F1 help screen and save game message
		{'<', L"\x25C4", 0x11},       // ◄ 'Enter' char 1
		{'/', L"\x2518", 0xD9},       // ┘ 'Enter' char 2

		// F1 help screen
		{'^', L"\x2191", 0x18},       // ↑ up (same as WEAPON)
		{'v', L"\x2193", 0x19},       // ↓ down
		{'>', L"\x2192", 0x1A},       // → right
		{'<', L"\x2190", 0x1B},       // ← left

		// F2 help screen
		{'#', L"\x2593", 0xB2},       // ▓ 'passage', different char

		{'`', L"`", 0}  // if ` appears on screen, something went wrong!
};

static CCODE btab[] = {
		// single-width box glyphs
		{'|', L"\x2502", VLINE},      // │
		{'-', L"\x2500", HLINE},      // ─
		{'.', L"\x250C", ULCORNER},   // ┌
		{'.', L"\x2510", URCORNER},   // ┐
		{'`', L"\x2514", LLCORNER},   // └
		{'\'',L"\x2518", LRCORNER},   // ┘

		// same as *WALL set, but used in boxes, with different ASCII
		{'H', L"\x2551", DVLINE},     // ║
		{'=', L"\x2550", DHLINE},     // ═
		{'#', L"\x2554", DULCORNER},  // ╔
		{'#', L"\x2557", DURCORNER},  // ╗
		{'#', L"\x255A", DLLCORNER},  // ╚
		{'#', L"\x255D", DLRCORNER},  // ╝

		// same as PASSAGE, used for curtain
		{'#', L"\x2592", FILLER},     // ▒

		{'\0', L"", 0}
};

/*@
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
static TTYSEQ ttymap[] = {
		{TTY_SS3 "j", '*'},
		{TTY_SS3 "k", '+'},
		{TTY_SS3 "m", '-'},
		{TTY_SS3 "n", '.'},
		{TTY_SS3 "o", '/'},
		{TTY_SS3 "M",  KEY_ENTER},
		{TTY_CSI "E",  KEY_B2},
		{TTY_CSI "1~", KEY_HOME},
		{TTY_CSI "4~", KEY_END},
};

/*@
 * Curses key codes the game understands, as rogue::ui::key values.
 * The game's own translation to commands is in ui/DosScreen.cpp.
 */
static const struct {
	int keycode;
	int key;
} keytab[] = {
	{KEY_ENTER,	key::Enter}, //@ Keypad Enter
	{KEY_HOME,	key::Home},
	{KEY_FIND,	key::Home},  //@ Keypad Home (7) in some terminals
	{KEY_A1,	key::Home},  //@ Keypad upper left (7)
	{KEY_UP,	key::Up},
	{KEY_PPAGE,	key::PageUp},  //@ Page Up
	{KEY_A3,	key::PageUp},  //@ Keypad upper right (9)
	{KEY_BACKSPACE, key::Backspace},
	{KEY_LEFT,	key::Left},
	{KEY_RIGHT,	key::Right},
	{KEY_END,	key::End},
	{KEY_SELECT,	key::End},  //@ Keypad End (1) in some terminals
	{KEY_C1,	key::End},  //@ Keypad lower left (1)
	{KEY_DOWN,	key::Down},
	{KEY_NPAGE,	key::PageDown},  //@ Page Down
	{KEY_C3,	key::PageDown},  //@ Keypad lower right (3)
	{KEY_IC,	key::Insert},  //@ Insert
	{KEY_DC,	key::Delete},  //@ Delete
	{KEY_F(1),	key::function(1)},
	{KEY_F(2),	key::function(2)},
	{KEY_F(3),	key::function(3)},
	{KEY_F(4),	key::function(4)},
	{KEY_F(5),	key::function(5)},
	{KEY_F(6),	key::function(6)},
	{KEY_F(7),	key::function(7)},
	{KEY_F(8),	key::function(8)},
	{KEY_F(9),	key::function(9)},
	{KEY_F(57),	key::AltF9}  //@ ALT+F9
};


#ifdef ROGUE_WIDECHAR
cchar_t *
unicode_from_dos(byte chd, byte dos_attr, CCODE *mapping)
{
	short color;
	attr_t attrs;

	CCODE *ccp = charcode_from_dos(chd, mapping);
	attrw_from_dos(dos_attr, &attrs, &color);

	setcchar(&cctemp,
			ccp->unicode,
			attrs,
			color,
			NULL);
	return &cctemp;
}
#endif  // ROGUE_WIDECHAR


void
define_keys(void)
{
#ifdef NCURSES_VERSION
	int i;
	int shift;
	TTYSEQ *ptr, *max;

	// get the shift offset of the bit past KEY_MAX
	for (i=KEY_MAX, shift=1; i>>=1; shift++);

	// define the mask that will be used by cur_getch() and friends
	KEY_MASK = (1 << shift) - 1;

	// define keys. first key gets i>0 to leave room for user terminfo keys
	for (i=8, ptr=ttymap, max=ASIZE(ttymap); ptr < max; ptr++)
	{
		if(!key_defined(ptr->def))
		{
			define_key(ptr->def, ((i++) << shift) | ptr->dest);
		}
	}
#endif  // NCURSES_VERSION
}


byte
ascii_from_dos(byte chd, CCODE *mapping)
{
	return charcode_from_dos(chd, mapping)->ascii;
}


CCODE *
charcode_from_dos(byte chd, CCODE *mapping)
{
	CCODE *ccp;

	// Shortcut for "ordinary" chars that map to themselves
	if (chd == '\0' || chd == '\n' || (isascii(chd) && isprint(chd)))
	{
		ccode.ascii = ccode.dos = ccunicode[0] = chd;  //@ ccode.unicode points to ccunicode
		return &ccode;
	}

	for(ccp = mapping; ccp->dos; ccp++)
	{
		if (chd == ccp->dos)
		{
			return ccp;
		}
	}
	// if not found, will return the sentinel
	return ccp;
}

/*
 * Return a color index, based on DOS char attribute.
 * For now index is in range 0-7, and the interpretation of A_DOS_BRIGHT bit to
 * either `fg index += 8` or `color | [W]A_BOLD` is the caller's responsibility.
 * This could be changed in the future.
 */
short
color_from_dos(byte dos_attr, bool fg)
{
	byte color = (dos_attr >> (fg ? A_DOS_FG_COLOR : A_DOS_BG_COLOR)) & \
			A_DOS_COLOR_MASK;

	// swap red and blue components so DOS index match ANSI's
	return swap_bits(color, 0, 2, 1);
}


/*
 * Convert a DOS/CGA character attribute to its curses equivalent
 *
 * The attribute model used by addch() and attrset() has no distinct type for
 * attributes: addch() expects a chtype OR'ed with A_* constants, and attrset()
 * expects an int, hinting that all A_* constants, as well as any chtype AND'ed
 * with A_ATTRIBUTES, fit int range.
 *
 * Return type chtype was chosen for consistency with termattrs() and vidattr(),
 * which are the only known functions dealing with an isolated set of OR'ed A_*
 * attributes. int could also have been chosen, as per attrset() usage.
 *
 * Return type could be attr_t, a dedicated type for attributes. In ncurses it
 * is typedef'd to chtype, but they are semantically different: attr_t expects
 * to be manipulated using the WA_* constants, and it's not expected to contain
 * color pair information: functions with a attr_t argument also have another
 * argument for color pair of type short.
 *
 * The only functions that deal exclusively with the attr_t model and have no
 * counterpart using the old model are the ones working with cchar_t wide chars
 * ("complex renditions" in ncurses docs). For those there is attrw_from_dos()
 */
chtype
attr_from_dos(byte dos_attr)
{
	chtype attr = A_NORMAL | COLOR_PAIR(0);
	short fg, bg;

	// shortcut to avoid setting (and calculating) a spurious color pair
	if (dos_attr == A_DOS_NORMAL)
		return attr;

	if (dos_attr & A_DOS_BLINK)
		attr |= A_BLINK;

	fg = color_from_dos(dos_attr, TRUE);
	bg = color_from_dos(dos_attr, FALSE);

	if (dos_attr & A_DOS_BRIGHT)
	{
		if (colors < 16)
			attr |= A_BOLD;
		else
			fg += 8;
	}

#ifdef NCURSES_VERSION
	/*
	 * Set terminal reverse attribute when Rogue implies it
	 *
	 * CGA does not have a "reverse" mode, Rogue achieves it by manually
	 * setting a white background with black foreground (black by default,
	 * but foreground could also be set to other colors, see cur_addch()).
	 * Thus, A_DOS_STANDOUT require no special handling and could be treated
	 * like any other color pair, and this block is entirely optional.
	 *
	 * By activating the terminal reverse mode and swapping fg with bg to
	 * revert Rogue's reversal, we allow the default fg/bg terminal colors
	 * to be used instead of hard-coded black on white, making reversed
	 * A_DOS_STANDOUT text consistent with A_DOS_NORMAL text even if user's
	 * terminal color theme is different from Rogue's default.
	 *
	 * This does not work with 8-color terminals: on reversed mode, A_BOLD
	 * operates on the background color, making it impossible to get yellow
	 * as foreground.
	 */
	if (((dos_attr & A_DOS_STANDOUT) == A_DOS_STANDOUT)
			&& colors != 8
			&& use_terminal_fgbg)
	{
		attr |= A_REVERSE;

		short tmp = bg;
		bg = fg;
		fg = tmp;
	}
#endif  // NCURSES_VERSION

	/*
	 * BIG problem here: if colors == 0, we should not use color pairs at
	 * all, but map the entries in monoc_attr from original intentions to
	 * current curses A_* attributes like underline, bold, standout, etc.
	 */
	if (colors > 0)
		attr |= COLOR_PAIR_N(fg, bg);

	return attr;
}


#ifdef ROGUE_WIDECHAR
void
attrw_from_dos(byte dos_attr, attr_t *attrs, short *color_pair)
{
	/*
	 * A sloppy version could simply assume that attr_t is typedef'd to
	 * chtype and all WA_* == A_*, which is true for current ncurses,
	 * and this function would be simplified to:
	 *
	 * attr_t bute = attr_from_dos(dos_attr);
	 * *attrs = bute & A_ATTRIBUTES & ~A_COLOR,
	 * *color_pair = PAIR_NUMBER(bute);
	 *
	 * Tempting, but we shall not make such assumptions. By the book, boys!
	 */

	short fg, bg;

	*attrs = WA_NORMAL;
	*color_pair = 0;

	if (dos_attr == A_DOS_NORMAL)
		return;

	if (dos_attr & A_DOS_BLINK)
		*attrs |= WA_BLINK;

	fg = color_from_dos(dos_attr, TRUE);
	bg = color_from_dos(dos_attr, FALSE);

	if (dos_attr & A_DOS_BRIGHT)
	{
		if (colors < 16)
			*attrs |= WA_BOLD;
		else
			fg += 8;
	}

#ifdef NCURSES_VERSION
	if (((dos_attr & A_DOS_STANDOUT) == A_DOS_STANDOUT)
			&& colors != 8
			&& use_terminal_fgbg)
	{
		*attrs |= WA_REVERSE;

		short tmp = bg;
		bg = fg;
		fg = tmp;
	}
#endif  // NCURSES_VERSION

	if (colors > 0)
		*color_pair = PAIR_INDEX(fg, bg);
}
#endif  // ROGUE_WIDECHAR


void
init_curses_colors(void)
{
	int fg, dos_fg, dfg;
	int bg, dos_bg, dbg;
	int i, r, g, b, cube;
	int colormode;
	int cmap[16];

	/*
	 * Not sure if this test should include bwflag, as set via env file.
	 * Original winit() doesn't, as it only cares about actual *hardware*
	 * capabilities. But on modern machines bwflag is the only way for
	 * players to simulate a bw hardware monitor like TTL or IBM's MDA,
	 * which is very different from simply "using no colors": it had bright,
	 * underline, blink, etc. We may consider a way to set this "hardware
	 * bw monitor" mode even if terminal supports colors.
	 */
	if (!has_colors() || COLORS < 8)
	{
		colors = 0;
		return;
	}

	/*
	 * Some notes on colors and mappings:
	 *
	 * DOS only uses 8 basic colors, and the foreground could be bumped to
	 * 16 via bright attribute. So for all code outside this function,
	 * background and foreground color indexes range from 0-15, so at most
	 * 16 * 16 = 256 color pairs are needed, and should be always accessed
	 * via PAIR_INDEX(fg, bg) or COLOR_PAIR_N(fg, bg) macros.
	 *
	 * Color indexes in DOS (actually, in CGA/VGA) are an RGB bitmap, blue
	 * being the least significant bit, so DOS colors have the Red and Blue
	 * components swapped compared to ANSI colors, which the curses named
	 * constants derive from. color_from_dos() takes care of this, so fg/bg
	 * indexes should be interpreted by ANSI table, ie, 1=Red, 4=Blue, etc
	 *
	 * The actual colors mapped to each of this 16 color indexes depends on
	 * terminal color capabilities:
	 * - If only 8, achieve the 16 via curses [W]A_BOLD attribute.
	 * - For 16 color terminals there's a 1:1 mapping
	 * - For 8 and 16, try to redefine terminal RGB values to match CGA
	 * - 88 and 256, remap the 16 color indexes to the 4x4x4 or 6x6x6 color
	 *   cube to get an exact CGA color match.
	 */

	// colormode is only used here, colors is global
	colors = 16;
	if      (COLORS >= 256) colormode = 256;
	else if (COLORS >=  88) colormode =  88;
	else if (COLORS >=  16) colormode =  16;
	else                    colormode = colors = 8;

	switch(colormode)
	{
	case 8:
	case 16:
		cube = 0;
		if (can_change_color() && change_colors)
		{
			colors_changed = TRUE;
		}
		break;
	case  88:
		cube = 4;
		break;
	case 256:
		cube = 6;
		break;
	}

	if (cube)
	{
		for (i = 0; i < colors; i++)
		{
			r = (cube - 1) * CGA_RED(i);
			g = (cube - 1) * CGA_GREEN(i);
			b = (cube - 1) * CGA_BLUE(i);
			cmap[i] = 16 + cube * cube * r + cube * g + b;
		}
	}
	else if (colors_changed)
	{
		for (i = 0; i < colors; i++)
		{
			init_color(i,
					1000 * CGA_RED(i),
					1000 * CGA_GREEN(i),
					1000 * CGA_BLUE(i));
			cmap[i] = i;  // 1:1 mapping
		}
	}
	else
	{
		for (i = 0; i < colors; i++)
		{
			cmap[i] = i;  // 1:1 mapping
		}
	}

	/*@
	 * More notes on color mappings and pairs:
	 *
	 * - Color pair 0 is not initialized, as per recommendation in curses
	 *   documentation, and it is only used when DOS attributes are set to
	 *   A_DOS_NORMAL, for example by cur_standend().
	 *
	 * - The default foreground and background colors used by DOS, as
	 *   defined by A_DOS_NORMAL, are mapped to (COLOR_WHITE, COLOR_BLACK).
	 *   If the curses implementation is ncurses and the user allows it,
	 *   they are mapped to (-1,-1), the default foreground and background
	 *   terminal colors.
	 */

	dos_fg = color_from_dos(A_DOS_NORMAL, TRUE);
	dos_bg = color_from_dos(A_DOS_NORMAL, FALSE);
	if ((A_DOS_NORMAL & A_DOS_BRIGHT) && colors > 8)
		dos_fg += 8;

	dfg = cmap[COLOR_WHITE];
	dbg = cmap[COLOR_BLACK];

#ifdef NCURSES_VERSION
	/*
	 * One could argue that change_colors == TRUE should imply
	 * use_terminal_fgbg == TRUE, but currently they are independent.
	 */
	if (use_terminal_fgbg)
	{
		use_default_colors();
		dfg = dbg = -1;
	}
#else
	use_terminal_fgbg = FALSE;
#endif  // NCURSES_VERSION

	for (bg = 0; bg < colors; bg++)
	{
		for (fg = colors - (bg ? 2 : 1); fg >= 0; fg--)
		{
			init_pair(PAIR_INDEX(fg, bg),
					(fg == dos_fg) ? dfg : cmap[fg],
					(bg == dos_bg) ? dbg : cmap[bg]);
		}
	}

#ifdef ROGUE_DEBUG
	int j;
	printw("COLOR TEST - Displayed colors should match [R,G,B] values\n");
	printw("Color mode: %d colors, using %s\n", colormode,
			cube ? "color cube" :
			colors_changed ? "RGB" :
			colors > 8 ? "ANSI 16" :
			"ANSI 8 + Bold");
	for (i = 0; i < 8; i++)
	{
		printw(" %d [%3d,%3d,%3d] #%02X%02X%02X ",
				i,
				(int)(255 * CGA_RED(i)),
				(int)(255 * CGA_GREEN(i)),
				(int)(255 * CGA_BLUE(i)),
				(int)(255 * CGA_RED(i)),
				(int)(255 * CGA_GREEN(i)),
				(int)(255 * CGA_BLUE(i))
		);
		for(j=15;j;j--) waddch(stdscr, '#' | COLOR_PAIR_N(i, 0));
		if (colors > 8)
			for(j=15;j;j--) waddch(stdscr, '#' | COLOR_PAIR_N(i + 8, 0));
		else
			for(j=15;j;j--) waddch(stdscr, '#' | COLOR_PAIR_N(i, 0) | A_BOLD);
		printw(" #%02X%02X%02X [%3d,%3d,%3d]\n",
				(int)(255 * CGA_RED(i+8)),
				(int)(255 * CGA_GREEN(i+8)),
				(int)(255 * CGA_BLUE(i+8)),
				(int)(255 * CGA_RED(i+8)),
				(int)(255 * CGA_GREEN(i+8)),
				(int)(255 * CGA_BLUE(i+8))
		);
	}
#endif  // ROGUE_DEBUG
}


void
resize_screen()
{
	if ((LINES != cur_LINES) || (COLS != cur_COLS))
	{
		if (resizeterm(cur_LINES, cur_COLS) == OK)
		{
			flushinp();  //@ eat up the generated KEY_RESIZE
		}
		else
		{
			fatal("Could not resize resize terminal to %u x %u\n",
					cur_COLS, cur_LINES);
		}
	}
}


namespace rogue::ui {

std::expected<void, std::string>
CursesTerminal::open(int rows, int cols)
{
	if (open_)
		return {};

	cur_LINES = rows;
	cur_COLS = cols;

	setenv("ESCDELAY", "25", FALSE);
	initscr();
	open_ = true;
	if ((LINES < cur_LINES) || (COLS < cur_COLS))
	{
		std::string error = std::format(
				"{}-column mode requires at least a {} x {} screen\n"
				"Your terminal size is {} x {}\n",
				cur_COLS, cur_COLS, cur_LINES, COLS, LINES);
		close();
		return std::unexpected(std::move(error));
	}
	start_color();
	cbreak();  //@ do not buffer input until ENTER
	noecho();  //@ do not echo typed characters
	nodelay(stdscr, FALSE); //@ use a blocking getch() (already the default)
	keypad(stdscr, TRUE);   //@ enable directional arrows, keypad, home, etc

	resize_screen();
	define_keys();
	init_curses_colors();
	return {};
}

void
CursesTerminal::close()
{
	if (!open_)
		return;
	/*
	 * Curses resets color RGB based on terminfo, which is somewhat
	 * useless, as (1) few terminals have terminfo default colors
	 * entries (linux does, xterm does not), and (2) those terminfo
	 * colors might not be the current ones before game start: user
	 * might have themed the terminal in .bashrc, .Xresources, etc.
	 *
	 * So we have 2 choices: we can redefine colors back to ANSI's
	 * default RGB, which is also useless on (2), or we can try
	 * `system("type reset 2>/dev/null && reset");`, which reset
	 * colors on some terminals (xterm, but not gnome-terminal)
	 *
	 * There's also a 3rd choice: do nothing! After all, user told
	 * us to change_colors = TRUE, didn't they? So we did it :)
	 */
	endwin();
	open_ = false;
}

bool
CursesTerminal::has_color() const
{
	return colors > 0;
}

/*@
 * Paint one cell. Line cells (boxes, curtain, repchr()) look up the box
 * glyph table first, which has different ASCII fallbacks.
 *
 * The *chnstr functions neither advance the cursor nor wrap, so the
 * bottom-right corner can be written like any other cell.
 */
void
CursesTerminal::draw(int row, int col, const Cell &cell)
{
	CCODE *mapping = cell.line ? btab : ctab;

	switch (charset)
	{
	default:
	case ASCII:
	{
		chtype ch = ascii_from_dos(cell.ch, mapping);
		if (ch == '\0')
			ch = ascii_from_dos(cell.ch, ctab);
		ch |= attr_from_dos(cell.attr);
		mvwaddchnstr(stdscr, row, col, &ch, 1);
		break;
	}
	case CP437:
	{
		chtype ch = cell.ch | attr_from_dos(cell.attr);
		mvwaddchnstr(stdscr, row, col, &ch, 1);
		break;
	}
#ifdef ROGUE_WIDECHAR
	case UNICODE:
	{
		cchar_t *cch = unicode_from_dos(cell.ch, cell.attr, mapping);
		if (cch->chars[0] == L'\0')
			cch = unicode_from_dos(cell.ch, cell.attr, ctab);
		mvwadd_wchnstr(stdscr, row, col, cch, 1);
		break;
	}
#endif  // ROGUE_WIDECHAR
	}
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

/*@
 * Originally in dos.asm, which used hardware port 0x61 (Keyboard
 * Controller) for direct PC Speaker access.
 */
void
CursesTerminal::bell()
{
	beep();
}

/*@
 * Read a key from user input. getch() with non-blocking capability
 *
 * timeout_ms has same meaning as delay in timeout(): If no key was pressed
 * after that many milliseconds, return key::None. Negative values will block
 * until a key is pressed. wgetch() refreshes the screen first.
 *
 * After the wgetch() call, input will always restore to blocking mode using
 * nodelay(FALSE);
 *
 * Return key::None on non-ASCII chars, on window resize and on keys the game
 * does not use.
 */
int
CursesTerminal::read_key(int timeout_ms)
{
	int ch;

	wtimeout(stdscr, timeout_ms);
#ifdef ROGUE_WIDECHAR
	wint_t wchi;
	int ret;
	if ((ret = wget_wch(stdscr, &wchi)) == ERR || (ret == OK && !isascii(wchi)))
	{
		// we're only interested in ASCII input
		ch = ERR;
	}
	else
	{
		// KEY_* codes always fit int, so no need for any special test
		ch = (int)wchi;
	}
#else
	ch = wgetch(stdscr);
#endif  // ROGUE_WIDECHAR
	nodelay(stdscr, FALSE);

	if (ch == ERR)
		return None;

	// mask-map custom keys
	ch = KEY_MASK & ch;

	// window resize needs special handling
	if (ch == KEY_RESIZE)
	{
		resize_screen();
		return None;
	}
	if (ch < KEY_MIN)
		return ch;
	for (const auto &k : keytab)
	{
		if (ch == k.keycode)
			return k.key;
	}
	return None;
}

} // namespace rogue::ui
