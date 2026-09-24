/*
 *  Cursor motion header for Monochrome display
 */

/*@
 * Contains only the curses-related declarations needed for the game files.
 *
 * Headers used only by curses.c were moved to curses_dos.h
 * Headers used by both curses.c and game files were moved to curses_common.h
 * Headers not related to curses or provided externally were moved elsewhere.
 * Unused headers were removed.
 *
 * The terminal backend, ui/curses/CursesTerminal.cpp, shall NOT include
 * this header
 *
 * This is, along with the included curses_common.h header, is the curses
 * public API as used by the game.
 */

#include "curses_common.h"

#define stdscr	NULL
#define hw	stdscr
#define eatme	stdscr

//@ Original macros
#define	wclear	clear
#define mvwaddch(w,a,b,c)	mvaddch(a,b,c)
#define getyx(a,b,c)	getxy(&b,&c)
#define getxy	getrc

//@ Modified macros
#define inch	cur_inch
#define standend	cur_standend
#define standout	cur_standout
#define endwin	cur_endwin

//@ Function mappings
#define beep	cur_beep
#define move	cur_move
#define clear	cur_clear
#define clrtoeol	cur_clrtoeol
#define mvaddstr	cur_mvaddstr
#define mvaddch	cur_mvaddch
#define mvinch	cur_mvinch
#define addch	cur_addch
#define addstr	cur_addstr
#define box	cur_box
#define printw	cur_printw
#define getch	cur_getch  //@ no longer used
#define getch_timeout	cur_getch_timeout


/*@
 * Screen size. Fixed at 80x25 (see rogue::ui::Screen); these used to be the
 * ncurses globals of the same name. const gives them internal linkage, so
 * they do not clash with ncurses' own symbols.
 */
const int LINES = MAXLINES;
const int COLS = MAXCOLS;

/*@
 * Global variables declarations. All defined in ui/DosScreen.cpp
 */
extern int is_saved;
extern int scr_type;
