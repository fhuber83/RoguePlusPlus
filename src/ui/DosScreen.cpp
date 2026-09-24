/*@
 * What remains of the DOS screen API PC Rogue was written against: the text
 * attribute tables, the colours map glyphs get, and starting and stopping
 * the terminal. The drawing functions (cur_move, cur_addch, boxes, curtains,
 * getinfo, ...) were replaced by ui::Display and ui::Input in phase 4.
 */

#include "ui/Display.hpp"
#include "ui/Screen.hpp"
#include "ui/curses/CursesTerminal.hpp"

#include "curses_common.h"

using rogue::ui::Screen;
using rogue::ui::screen;
namespace dos = rogue::ui::dos;

#ifndef ROGUE_SCR_TYPE
#define ROGUE_SCR_TYPE 3  //@ 80x25 Color
#endif

int scr_type = -1;

static rogue::ui::CursesTerminal terminal;

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

/*@
 * The attribute cur_addch() gives glyph chr when the current attribute is
 * ch_attr: in colour mode, map glyphs get their own colours
 */
byte
glyph_attr(byte chr, byte ch_attr)
{
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

	return ch_attr;
}

/*@
 * The DOS attribute for a set_attr() index (a raw attribute passes through)
 */
byte
dos_attr(int bute)
{
	return bute < MAXATTR ? at_table[bute] : (byte)bute;
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

namespace rogue::ui {

void start_terminal()
{
	winit();
}

void stop_terminal()
{
	cur_endwin();
}

} // namespace rogue::ui
