/*
 * The IBM extended keys as command characters.
 *
 * mach_dep.c	1.4 (A.I. Design) 12/1/84
 */

#include "ui/Keys.hpp"

#include "glyphs.h"
#include "ui/Input.hpp"

namespace rogue::ui {

namespace {

/*
 * Table for IBM extended key translation, from rogue::ui::key values
 */
struct Translation {
	int keycode;
	unsigned char keyis;
};

constexpr Translation xtab[] = {
	{key::Enter,	'\n'}, // Keypad Enter
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
	{key::function(4),	ctrl('R')},
	{key::function(5),	'c'},
	{key::function(6),	'D'},
	{key::function(7),	'i'},
	{key::function(8),	'^'},
	{key::function(9),	ctrl('F')},
	{key::AltF9,	'F'}  // ALT+F9
};

}  // namespace

unsigned char
command_char(int key)
{
	for (const Translation &x : xtab)
		if (key == x.keycode)
			return x.keyis;
	return static_cast<unsigned char>(key);
}

}  // namespace rogue::ui
