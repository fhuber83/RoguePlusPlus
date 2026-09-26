/*
 * Screen size, glyph codes and key codes shared by the game and the screen
 * code. Glyphs are CP437 codes, as the original drew them into video memory;
 * the terminal backend maps them to Unicode or ASCII.
 */

#pragma once

/*
 * Don't change the constants, since they are used for sizes in many
 * places in the program. 80 and 25 are also hard coded in many places.
 */
inline constexpr int MAXSTR = 80;	/* maximum length of strings */
inline constexpr int MAXLINES = 25;	/* maximum number of screen lines used */
inline constexpr int MAXCOLS = 80;	/* maximum number of screen columns used */

// The key a control character is typed with: ctrl('R') is ^R
consteval unsigned char
ctrl(char ch)
{
	return static_cast<unsigned char>(ch & 037);
}

/*
 * Things that appear on the screens
 */
inline constexpr unsigned char PASSAGE = 0xb1;
inline constexpr unsigned char DOOR = 0xce;
inline constexpr unsigned char FLOOR = 0xfa;
inline constexpr unsigned char PLAYER = 0x01;
inline constexpr unsigned char TRAP = 0x04;
inline constexpr unsigned char STAIRS = 0xf0;
inline constexpr unsigned char GOLD = 0x0f;
inline constexpr unsigned char POTION = 0xad;
inline constexpr unsigned char SCROLL = 0x0d;
inline constexpr unsigned char MAGIC = '$';
inline constexpr unsigned char BMAGIC = '~';  // originally '+', which is the ASCII door
inline constexpr unsigned char FOOD = 0x05;
inline constexpr unsigned char STICK = 0xe7;
inline constexpr unsigned char ARMOR = 0x08;
inline constexpr unsigned char AMULET = 0x0c;
inline constexpr unsigned char RING = 0x09;
inline constexpr unsigned char WEAPON = 0x18;

inline constexpr unsigned char VWALL = 0xba;
inline constexpr unsigned char HWALL = 0xcd;
inline constexpr unsigned char ULWALL = 0xc9;
inline constexpr unsigned char URWALL = 0xbb;
inline constexpr unsigned char LLWALL = 0xc8;
inline constexpr unsigned char LRWALL = 0xbc;

// single-width box glyphs
inline constexpr unsigned char HLINE = 0xc4;
inline constexpr unsigned char VLINE = 0xb3;
inline constexpr unsigned char ULCORNER = 0xda;
inline constexpr unsigned char URCORNER = 0xbf;
inline constexpr unsigned char LLCORNER = 0xc0;
inline constexpr unsigned char LRCORNER = 0xd9;

// double-width box glyphs
inline constexpr unsigned char DHLINE = HWALL;
inline constexpr unsigned char DVLINE = VWALL;
inline constexpr unsigned char DULCORNER = ULWALL;
inline constexpr unsigned char DURCORNER = URWALL;
inline constexpr unsigned char DLLCORNER = LLWALL;
inline constexpr unsigned char DLRCORNER = LRWALL;

// only used on the title screen
inline constexpr unsigned char DVLEFT = 0xb9;
inline constexpr unsigned char DVRIGHT = 0xcc;

inline constexpr unsigned char ESCAPE = 27;
