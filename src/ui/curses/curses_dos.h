/*@
 * Private headers of the curses terminal backend, CursesTerminal.cpp
 *
 * It is hereby considered private implementation details and as such it
 * should NOT be included by any game files.
 */

#include "ui/Cell.hpp"

//@ max is also in rogue.h
#define max(a,b)	((a) > (b) ? (a) : (b))
#define min(a,b)	((a) < (b) ? (a) : (b))

//@ DOS attribute bits, see rogue::ui::dos
#define A_DOS_BLACK    rogue::ui::dos::Black
#define A_DOS_BLUE     rogue::ui::dos::Blue
#define A_DOS_GREEN    rogue::ui::dos::Green
#define A_DOS_RED      rogue::ui::dos::Red
#define A_DOS_WHITE    rogue::ui::dos::White
#define A_DOS_BRIGHT   rogue::ui::dos::Bright
#define A_DOS_BLINK    rogue::ui::dos::Blink
#define A_DOS_NORMAL   rogue::ui::dos::Normal
#define A_DOS_STANDOUT rogue::ui::dos::Standout

#define A_DOS_FG_COLOR    0  // foreground color shift offset
#define A_DOS_BG_COLOR    4  // background color shift offset
#define A_DOS_COLOR_MASK  7  // to extract color after shifting

#define PAIR_INDEX(fg, bg)	(bg * colors + fg + 1)
#define COLOR_PAIR_N(fg, bg)	COLOR_PAIR(PAIR_INDEX(fg, bg))

/*
 * Original CGA colors
 * https://en.wikipedia.org/wiki/Color_Graphics_Adapter#Color_palette
 * red   := 2/3 * (colorNumber & 4)/4 + 1/3 * (colorNumber & 8)/8
 * green := 2/3 * (colorNumber & 2)/2 + 1/3 * (colorNumber & 8)/8
 * blue  := 2/3 * (colorNumber & 1)/1 + 1/3 * (colorNumber & 8)/8
 * if colorNumber = 6 then green := green / 2
 *
 * These macros swap Red and Blue components, so `c` is ANSI index (brown is 3)
 * Return a float in range [0, 1] (both ends included!)
 */
#define CGA_COMP(c, i)	(!!((c) & i) * 2 / 3.0 + !!((c) & 8) * 1 / 3.0)
#define CGA_RED(c)	 CGA_COMP(c, 1)
#define CGA_GREEN(c)	(CGA_COMP(c, 2) / ((c) == 3 ? 2 : 1))
#define CGA_BLUE(c)	 CGA_COMP(c, 4)

#define ALENGTH(arr)	(sizeof (arr) / sizeof (*arr))
#define ASIZE(arr)	(arr + ALENGTH(arr))

#define TTY_ESC "\033"
#define TTY_CSI TTY_ESC "["
#define TTY_SS3 TTY_ESC "O"

struct ttykeys {
	const char *def;
	int dest;
};
typedef struct ttykeys TTYSEQ;

struct charcode {
	byte ascii;
	const wchar_t *unicode;
	byte dos;
};
typedef struct charcode CCODE;

#ifdef ROGUE_WIDECHAR
cchar_t *unicode_from_dos(byte chd, byte dos_attr, CCODE *mapping);
void	attrw_from_dos(byte dos_attr, attr_t *attrs, short *color_pair);
#endif  // ROGUE_WIDECHAR
void	define_keys(void);
byte	ascii_from_dos(byte chd, CCODE *mapping);
CCODE	*charcode_from_dos(byte chd, CCODE *mapping);
short	color_from_dos(byte dos_attr, bool fg);
chtype	attr_from_dos(byte dos_attr);
void	init_curses_colors(void);
void	resize_screen();
