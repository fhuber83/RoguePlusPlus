/*@
 * Private header of the screen code in ui/ (DosScreen.cpp, ScreenDisplay.cpp,
 * curses/CursesTerminal.cpp): charsets, DOS attribute helpers and terminal
 * start/stop. Game files include "glyphs.h" through rogue.h instead.
 */

#pragma once

//@ not in original, to make IDE happy about 'bool'
#include "extern.h"
#include "glyphs.h"

//@ Available charsets, not in original
#define ASCII	1
#define CP437	2
#define UNICODE	3

//@ User-selected charset, also not in original
#if !(ROGUE_CHARSET == ASCII || ROGUE_CHARSET == CP437 || ROGUE_CHARSET == UNICODE)
	//@ Factory default charset
	#define ROGUE_CHARSET	UNICODE
#endif
//@ UNICODE is subject to curses wide char availability
#if ROGUE_CHARSET == UNICODE && !defined (_XOPEN_CURSES)
	#undef  ROGUE_CHARSET
	#define ROGUE_CHARSET	ASCII
#endif
//@ Only enable wide chars if actually needed
#if ROGUE_CHARSET == UNICODE
	#define ROGUE_WIDECHAR
#endif

/*@
 * This color/bw checks are inconsistent with each other:
 * scr_type 0 and 2 evaluate as TRUE for both (but they are mono),
 * scr_type 7 evaluate as FALSE for both (also mono)
 * See winit()
 */
#define is_color (scr_type!=7)
#define is_bw (scr_type==0 || scr_type==2)

extern int scr_type;

//@ only used in the curtain animation
#define FILLER	PASSAGE

//@ total time, in milliseconds, for each drop and raise curtain animation
#define CURTAIN_TIME	1500

/*@
 * Function prototypes, all in ui/DosScreen.cpp
 */
byte	dos_attr(int bute);
byte	glyph_attr(byte chr, byte ch_attr);
void	winit(void);
void	cur_endwin(void);
