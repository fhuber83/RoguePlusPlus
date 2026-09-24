/*@
 * Screen size, glyph codes and key codes shared by the game and the screen
 * code. Glyphs are CP437 codes, as the original drew them into video memory;
 * the terminal backend maps them to Unicode or ASCII.
 *
 * Split from curses_common.h in phase 4.5 so that game files need no part
 * of the DOS screen API.
 */

#pragma once

/*
 * Don't change the constants, since they are used for sizes in many
 * places in the program.
 * @ Heed the warning! 80 and 25 are hard coded in many places... sigh
 * @ moved from rogue.h
 */
#define MAXSTR  	80	/* maximum length of strings */
#define MAXLINES	25	/* maximum number of screen lines used */
#define MAXCOLS 	80	/* maximum number of screen columns used */

//@ moved from rogue.h
#ifndef CTRL
#define CTRL(ch)	((ch) & 037)
#endif

/*
 * Things that appear on the screens
 * @ moved from rogue.h
 */
#define PASSAGE		(0xb1)
#define DOOR		(0xce)
#define FLOOR		(0xfa)
#define PLAYER		(0x01)
#define TRAP		(0x04)
#define STAIRS		(0xf0)
#define GOLD		(0x0f)
#define POTION		(0xad)
#define SCROLL		(0x0d)
#define MAGIC		'$'
#define BMAGIC		'~'  //@ originally '+'. Reverse ASCII map must be unique
#define FOOD		(0x05)
#define STICK		(0xe7)
#define ARMOR		(0x08)
#define AMULET		(0x0c)
#define RING		(0x09)
#define WEAPON		(0x18)
#define CALLABLE	-1

#define VWALL	(0xba)
#define HWALL	(0xcd)
#define ULWALL	(0xc9)
#define URWALL	(0xbb)
#define LLWALL	(0xc8)
#define LRWALL	(0xbc)

//@ The following were not in original - values were hard-coded

//@ single-width box glyphs
#define HLINE	(0xc4)
#define VLINE	(0xb3)
#define CORNER	'+'  //@ unused, added just for completeness
#define ULCORNER	(0xda)
#define URCORNER	(0xbf)
#define LLCORNER	(0xc0)
#define LRCORNER	(0xd9)

//@ double-width box glyphs
#define DHLINE	HWALL  // 205 in credits()
#define DVLINE	VWALL
#define DCORNER	'#'  //@ also unused
#define DULCORNER	ULWALL
#define DURCORNER	URWALL
#define DLLCORNER	LLWALL
#define DLRCORNER	LRWALL

//@ only used in credits()
#define DVLEFT	(0xb9)  //@ 185
#define DVRIGHT	(0xcc)  //@ 204

//@ moved from rogue.h
#define ESCAPE	(27)

//@ same as ERR, but different semantics
#define NOCHAR	(-1)

