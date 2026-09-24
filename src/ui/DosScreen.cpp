/*@
 * The DOS screen API that PC Rogue was written against (cur_move, cur_addch,
 * set_attr, wdump, boxes, curtains, getinfo, ...), implemented on top of
 * rogue::ui::Screen. Game files reach it through the macros in the local
 * "curses.h".
 *
 * Moved here from curses.c, which is now only the terminal backend
 * (ui/curses/CursesTerminal.cpp). This file shrinks as the game moves to the
 * ui::Display interface.
 */

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "ui/Screen.hpp"
#include "ui/Terminal.hpp"
#include "ui/curses/CursesTerminal.hpp"

#include "curses_common.h"

using rogue::ui::Cell;
using rogue::ui::Screen;
using rogue::ui::screen;
namespace dos = rogue::ui::dos;
namespace key = rogue::ui::key;

#ifndef ROGUE_SCR_TYPE
#define ROGUE_SCR_TYPE 3  //@ 80x25 Color
#endif

/*
 *  Globals for curses
 *  (extern'ed in curses.h)
 */
int is_saved = FALSE;  //@ in practice, TRUE disables status updates in SIG2()
int scr_type = -1;

static rogue::ui::CursesTerminal terminal;

static Screen::Snapshot savewin;  //@ wdump()/wrestor() buffer
static Screen::Snapshot curtain;  //@ drop_curtain()/raise_curtain() buffer

/*@
 * Original used decimal literals for both tables
 */
#define MAXATTR 17
static const byte color_attr[] = {
	dos::Normal,                  /*  0 normal         */
	dos::Green,                   /*  1 green          */
	dos::Cyan,                    /*  2 cyan           */
	dos::Red,                     /*  3 red            */
	dos::Magenta,                 /*  4 magenta        */
	dos::Brown,                   /*  5 brown          */
	dos::Bright | dos::Black,     /*  6 dark grey      */
	dos::Bright | dos::Blue,      /*  7 light blue     */
	dos::Bright | dos::Green,     /*  8 light green    */
	dos::Bright | dos::Red,       /*  9 light red      */
	dos::Bright | dos::Magenta,   /* 10 light magenta  */
	dos::Bright | dos::Brown,     /* 11 yellow         */
	dos::Bright | dos::White,     /* 12 uline          */
	dos::Blue,                    /* 13 blue           */
	dos::Standout,                /* 14 reverse        */
	dos::Bright | dos::Normal,    /* 15 high intensity */
	dos::Standout,                /* bold              */
	0                             /* no more           */
} ;

/*@
 * Reverse and Bold (standout(), bold()) are set differently than their color
 * table counterparts, using dark gray ("light black") as foreground. Visually
 * the difference is minor, but perhaps it was also meant to circumvent the
 * cur_addch() processing of dos::Standout used for passages/mazes.
 *
 * And surprisingly high()/set_attr(15) is set to normal white (ie, light gray)
 */
static const byte monoc_attr[] = {
	dos::Normal,      /*  0 normal         */
	dos::Normal,      /*  1 green          */
	dos::Normal,      /*  2 cyan           */
	dos::Normal,      /*  3 red            */
	dos::Normal,      /*  4 magenta        */
	dos::Normal,      /*  5 brown          */
	dos::Normal,      /*  6 dark grey      */
	dos::Normal,      /*  7 light blue     */
	dos::Normal,      /*  8 light green    */
	dos::Normal,      /*  9 light red      */
	dos::Normal,      /* 10 light magenta  */
	dos::Normal,      /* 11 yellow         */
	dos::BwUnderline, /* 12 uline          */
	dos::Normal,      /* 13 blue           */
	dos::BwStandout,  /* 14 reverse        */
	dos::Normal,      /* 15 white/hight    */
	dos::BwStandout,  /* 16 bold           */
	0                 /* no more           */
} ;

static const byte *at_table = color_attr;

enum { BX_UL, BX_UR, BX_LL, BX_LR, BX_VW, BX_HT, BX_HB, BX_SIZE };

static const byte dbl_box[BX_SIZE] = {
	DULCORNER, DURCORNER, DLLCORNER, DLRCORNER, DVLINE, DHLINE, DHLINE
};

static const byte sng_box[BX_SIZE] = {
	ULCORNER, URCORNER, LLCORNER, LRCORNER, VLINE, HLINE, HLINE
};

static const byte spc_box[BX_SIZE] = {
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20
};

/*
 * Table for IBM extended key translation
 * moved from march_dep.c
 */
static const struct xlate {
	int keycode;
	byte keyis;
} xtab[] = {
	{key::Enter,	'\n'}, //@ Keypad Enter
	{key::Home,	'y'},
	{key::Up,	'k'},
	{key::PageUp,	'u'},
	{key::Backspace, 'h'},
	{key::Left,	'h'},
	{key::Right,	'l'},
	{key::End,	'b'},
	{key::Down,	'j'},
	{key::PageDown,	'n'},
	{key::Insert,	'>'},
	{key::Delete,	's'},
	{key::function(1),	'?'},
	{key::function(2),	'/'},
	{key::function(3),	'a'},
	{key::function(4),	CTRL('R')},
	{key::function(5),	'c'},
	{key::function(6),	'D'},
	{key::function(7),	'i'},
	{key::function(8),	'^'},
	{key::function(9),	CTRL('F')},
	{key::AltF9,	'F'}  //@ ALT+F9
};


/*@
 * Beep a an audible beep, if possible
 *
 * Originally in dos.asm
 */
void
cur_beep(void)
{
	screen().bell();
}


/*@
 * Read a key, waiting at most msdelay milliseconds (forever if negative).
 * Return NOCHAR if none arrived. See rogue::ui::Terminal::read_key()
 */
int
cur_getch_timeout(int msdelay)
{
	return screen().read_key(msdelay);
}


/*@
 * Map a key to an 8-bit character using the game translation table.
 *
 * Moved from mach_dep.c as part of readchar()
 */
byte
xlate_ch(int ch)
{
	for (const struct xlate *x = xtab; x < xtab + (sizeof xtab) / sizeof *xtab; x++)
	{
		if (ch == x->keycode)
			return x->keyis;
	}
	return (byte)ch;
}


/*@
 * Move the cursor to the given row and column
 *
 * Originally in zoom.asm
 *
 * Return 0, or -1 (leaving the cursor alone) when outside the screen
 */
int
cur_move(int row, int col)
{
	return screen().set_cursor(row, col) ? 0 : -1;
}


/*@
 * Return character (without any attributes) at current cursor position
 *
 * Originally in zoom.asm by the name curch(), which read it back from video
 * memory. The Screen grid plays that part again, so this is exact: the
 * curses port had to reverse-map terminal characters, which in ASCII mode
 * could not tell a corner from a wall.
 */
byte
cur_inch(void)
{
	return screen().at(screen().row(), screen().col()).ch;
}


/*
 * clear screen
 */
void
cur_clear(void)
{
	screen().erase();
}


/*
 *  Turn cursor on and off
 */
bool
cursor(bool ison)
{
	return screen().show_cursor(ison);
}


/*
 * get curent cursor position
 */
void
getrc(int *rp, int *cp)
{
	*rp = screen().row();
	*cp = screen().col();
}


//@ Not in original
void
cur_refresh(void)
{
	screen().refresh();
}

/*
 *	clrtoeol
 */
void
cur_clrtoeol(void)
{
	screen().erase_to_eol();
}

void
cur_mvaddstr(int r, int c, const char *s)
{
	cur_move(r, c);
	cur_addstr(s);
}

void
cur_mvaddch(int r, int c, byte chr)
{
	cur_move(r, c);
	cur_addch(chr);
}

byte
cur_mvinch(int r, int c)
{
	cur_move(r, c);
	return cur_inch();
}


/*
 * put the character on the screen and update the
 * character position
 */
void
cur_addch(byte chr)
{
	byte ch_attr = screen().attr();

	if (at_table == color_attr)
	{
		/* if it is inside a room */
		if (ch_attr == dos::Normal)
		{
			switch(chr)
			{
			case DOOR:
			case VWALL:
			case HWALL:
			case ULWALL:
			case URWALL:
			case LLWALL:
			case LRWALL:
				ch_attr = dos::Brown;  /* brown */
				break;
			case FLOOR:
				ch_attr = dos::Green | dos::Bright;  /* light green */
				break;
			case STAIRS:
				ch_attr = dos::Black | dos::background(dos::Green) | dos::Blink; /* black on green */
				break;
			case TRAP:
				ch_attr = dos::Magenta;  /* magenta */
				break;
			case GOLD:
			case PLAYER:
				ch_attr = dos::Yellow;  /* yellow */
				break;
			case POTION:
			case SCROLL:
			case STICK:
			case ARMOR:
			case AMULET:
			case RING:
			case WEAPON:
				ch_attr = dos::Blue | dos::Bright;
				break;
			case FOOD:
				ch_attr = dos::Red;
				break;
			}
		}
		/* if inside a passage or a maze */
		else if (ch_attr == dos::Standout)
		{
			switch(chr)
			{
			case FOOD:
				ch_attr = dos::Red | dos::Standout ;  /* red @ on white */
				break;
			case GOLD:
			case PLAYER:
				ch_attr = dos::Yellow | dos::Standout;  /* yellow on white */
				break;
			case POTION:
			case SCROLL:
			case STICK:
			case ARMOR:
			case AMULET:
			case RING:
			case WEAPON:
				ch_attr = dos::Blue | dos::Standout;  /* blue on white */
				break;
			}
		}
		//@ I suspect STAIRS used with high() is a case that never happen...
		else if (ch_attr == (dos::Bright | dos::Normal) && chr == STAIRS)
			ch_attr = dos::Black | dos::background(dos::Green) | dos::Blink;
	}

	screen().put(chr, ch_attr);
}


void
cur_addstr(const char *s)
{
	while(*s)
		cur_addch(*s++);
}


void
set_attr(int bute)
{
	if (bute < MAXATTR)
		screen().set_attr(at_table[bute]);
	else
		screen().set_attr((byte)bute);
}


/*
 *  winit(win_name):
 *		initialize window -- open disk window
 *						  -- determine type of moniter
 *						  -- determine screen memory location for dma
 *
 *  @ Starts the curses terminal and connects it to the screen.
 */
void
winit(void)
{
	if (terminal.is_open())
		return;

	/*@
	 * scr_type is only used by the game for is_color/is_bw, with ambiguous
	 * meanings. ROGUE_SCR_TYPE does not affect columns or colors.
	 */
	scr_type = ROGUE_SCR_TYPE;

	if (auto opened = terminal.open(Screen::Rows, Screen::Cols); !opened)
		fatal("%s", opened.error().c_str());

	/*@
	 * The only common code in winit() for both old and new curses.
	 * it was scattered after all winit() calls, so moved here.
	 * This replaces disabled forcebw()
	 */
	at_table = (terminal.has_color() && !bwflag) ? color_attr : monoc_attr;
	screen().connect(&terminal);
}

/*@
 * Dump the screen to the savewin buffer
 */
void
wdump(void)
{
	savewin = screen().snapshot();
	is_saved = TRUE;
}

/*@
 * Restore the screen from the savewin buffer
 */
void
wrestor(void)
{
	screen().restore(savewin);
	screen().refresh();
	is_saved = FALSE;
}

/*
 *   close the window file
 *   @renamed from wclose()
 */
void
cur_endwin()
{
	screen().connect(nullptr);
	terminal.close();
}

/*
 *  box:  draw a box using given the
 *        upper left coordinate and the lower right
 */
static void
vbox(const byte box[BX_SIZE], int ul_r, int ul_c, int lr_r, int lr_c)
{
	Screen &s = screen();
	bool wason = s.show_cursor(false);
	int i;

	i = (lr_c - ul_c - 1); s.line(ul_r, ul_c+1, box[BX_HT], i, false);
	                       s.line(lr_r, ul_c+1, box[BX_HB], i, false);
	i = (lr_r - ul_r - 1); s.line(ul_r+1, ul_c, box[BX_VW], i, true);
	                       s.line(ul_r+1, lr_c, box[BX_VW], i, true);

	s.line(ul_r, ul_c, box[BX_UL], 1, false);
	s.line(ul_r, lr_c, box[BX_UR], 1, false);
	s.line(lr_r, ul_c, box[BX_LL], 1, false);
	s.line(lr_r, lr_c, box[BX_LR], 1, false);
	s.show_cursor(wason);
}

void
cur_box(int ul_r, int ul_c, int lr_r, int lr_c)
{
	vbox(dbl_box, ul_r, ul_c, lr_r, lr_c);
}

/*
 * center a string according to how many columns there really are
 */
void
center(int row, const char *string)
{
	cur_mvaddstr(row, (Screen::Cols - (int)strlen(string)) / 2, string);
}


/*
 * printw(Ieeeee)
 */
void
cur_printw(const char *msg, ...)
{
	char pwbuf[132];
	va_list argp;

	va_start(argp, msg);
	vsnprintf(pwbuf, sizeof(pwbuf), msg, argp);
	va_end(argp);
	cur_addstr(pwbuf);
}


/*@
 * Repeat a character cnt times, advancing the cursor
 * Use current attribute, and do not go through cur_addch() processing
 */
void
repchr(byte chr, int cnt)
{
	Screen &s = screen();
	int c_row = s.row(), c_col = s.col();
	s.line(c_row, c_col, chr, cnt, false);
	s.set_cursor(c_row, c_col + cnt);
}

/*
 * Clear the screen in an interesting fashion
 */
void
implode()
{
	Screen &s = screen();
	int j, delay, r, c, cinc = Screen::Cols/10/2, er, ec;

	er = Screen::Rows-3;
	delay = 50;
	for (r = 0,c = 0,ec = Screen::Cols-1; r < 10; r++,c += cinc,er--,ec -= cinc) {
		vbox(sng_box, r, c, er, ec);
		s.refresh();
		msleep(delay);
		for (j = r+1; j <= er-1; j++) {
			s.line(j, c+1, ' ', cinc-1, false);
			s.line(j, ec-cinc+1, ' ', cinc-1, false);
		}
		vbox(spc_box, r, c, er, ec);
	}
	s.refresh();
}


/*@
 * Display a curtain down animation, keep it in the curtain buffer and clear
 * the screen without showing it. Whatever is drawn next stays hidden until
 * the next refresh, which raise_curtain() does line by line.
 */
void
drop_curtain(void)
{
	Screen &s = screen();
	int r;
	int delay = CURTAIN_TIME / Screen::Rows;

	cursor(FALSE);
	green();
	vbox(sng_box, 0, 0, Screen::Rows-1, Screen::Cols-1);
	s.refresh();
	msleep(delay);  // not in original
	yellow();
	for (r = 1; r < Screen::Rows-1; r++) {
		s.line(r, 1, FILLER, Screen::Cols-2, false);
		s.refresh();
		msleep(delay);
	}
	curtain = s.snapshot();
	msleep(delay);  // not in original, optional
	cur_move(0,0);
	cur_standend();
	s.erase();
}


/*@
 * Display a curtain up animation and re-enable screen refresh
 */
void
raise_curtain(void)
{
	Screen &s = screen();
	int line;
	int delay = CURTAIN_TIME / Screen::Rows;

	// save current screen
	Screen::Snapshot shown = s.snapshot();

	// restore and display the curtain
	for (line = 0; line < Screen::Rows; line++)
		s.restore_row(curtain, line);

	// progressively restore screen
	for (line = Screen::Rows-1; line >= 0; line--)
	{
		s.restore_row(shown, line);
		s.refresh();
		msleep(delay);
	}
	is_saved = FALSE;
}


/*
 * This routine reads information from the keyboard
 * It should do all the strange processing that is
 * needed to retrieve sensible data from the user
 *
 * @ "Strange processing" indeed:
 * - ESCAPE abort the input, set the first character of str to ESCAPE but leave
 *   all other typed characters there. It does *NOT* null-terminate str!!!
 *   Like in printw(), it couldn't care less about buffer exploits.
 *   Return ESCAPE.
 * - '\n' finishes input and null-terminate str. '\n' is not included in str.
 *   Return '\n'
 * - A non-ascii char (>127) also finishes input, but it *does* get included
 *   in str, probably unintentionally. srt is properly null-terminated.
 *   Return the non-ascii char.
 * - All other chars are accepted as normal input, including symbols (< 32).
 *
 * Original behavior is changed:
 * - Aborted input are null-terminated (ESCAPE + '\0')
 * - Only printable ASCII chars accepted (32 <= ch <= 126). This is universally
 *   compatible, until proper CP437 and UTF-8 support is implemented.
 *
 * In a sane, safe API this function would return a bool, FALSE if aborted
 * by ESCAPE and TRUE otherwise. In case of abortion, str could either
 * keep typed string or set first char to '\0', effectively blanking str.
 */
int
getinfo(char *str, int size)
{
	char *retstr;
	int ch;
	int readcnt = 0;
	int wason, ret = 1;
	retstr = str;
	*str = 0;
	wason = cursor(TRUE);
	while(ret == 1)
	{
		//@ Blocking read is fine, as SIG2() is not called anyway
		while ((ch = screen().read_key(-1)) == key::None);
		switch(ch)
		{
			case ESCAPE:
				while(str != retstr) {
					backspace();
					readcnt--;
					str--;
				}
				//@ null-termination was not in original
				ret = *str++ = ESCAPE;
				*str = 0;
				cursor(wason);
				break;
			case key::Backspace:
			case '\b':
				if (str != retstr) {
					backspace();
					readcnt--;
					str--;
				}
				break;
			default:
				if ( readcnt >= size) {
					cur_beep();
					break;
				}
				if (ch > 0x7f || !isprint(ch))
				{
					break;
				}
				readcnt++;
				cur_addch(ch);
				*str++ = ch;
				break;
			case key::Enter:
			case '\n':
				*str = 0;
				cursor(wason);
				ret = ch;  //@ any value different than ESCAPE or 1 would do.
				break;
		}
	}
	return ret;
}

/*@
 * Step back and blank the character under the cursor
 */
void
backspace(void)
{
	Screen &s = screen();
	if (s.col() > 0)
		s.set_cursor(s.row(), s.col() - 1);
	s.set(s.row(), s.col(), Cell{});
}
