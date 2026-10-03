/*
 * The help screens.
 *
 * help() comes from misc.c, the tables from extern.c.
 *
 * misc.c	1.4		(A.I. Design)	12/14/84
 * @(#)extern.c	5.2 (Berkeley) 6/16/82
 */

#include "game/Help.hpp"

#include <array>
#include <iterator>
#include <span>

#include "core/Glyphs.hpp"
#include "game/Game.hpp"
#include "game/Keyboard.hpp"
#include "ui/Display.hpp"

namespace rogue {

/*
 * Original code used CP437 codes hard coded inside the help strings,
 * instead of the #define'd char constants for FLOOR, PLAYER etc.
 * To support the constants, helpcoms/helpobjs array type has changed from
 * string to HelpLine (was struct h_list), whose constructors build the
 * glyph column.
 *
 * Ironically, struct h_list already existed in rogue.h, but it was unused in
 * code, so perhaps original authors either abandoned the idea or were halfway
 * through implementing it.
 */
const std::array<HelpLine, 62> helpcoms = {{
	{"F1     list of commands"},
	{"F2     list of symbols"},
	{"F3     repeat command"},
	{"F4     repeat message"},
	{"F5     rename something"},
	{"F6     recall what's been discovered"},
	{"F7     inventory of your possessions"},
	{"F8     <dir> identify trap type"},
	{"F9     The Any Key (definable)"},
	{"Alt F9 defines the Any Key"},
	{"Space  Clear -More- message"},
	{"\x11\xd9     the Enter Key"},
	{"\x1b      left"},
	{"\x19      down"},
	{"\x18      up"},
	{"\x1a      right"},
	{"Home   up & left"},
	{"PgUp   up & right"},
	{"End    down & left"},
	{"PgDn   down & right"},
	{"Scroll Fast Play mode"},
	{".      rest"},
	{">      go down a staircase"},
	{"<      go up a staircase"},
	{"Esc    cancel command"},
	{"d      drop object"},
	{"e      eat food"},
	{"f      <dir> find something"},
	{"q      quaff potion"},
	{"r      read paper"},
	{"s      search for trap/secret door"},
	{"t      <dir> throw something"},
	{"w      wield a weapon"},
	{"z      <dir> zap with a wand"},
	{"B      run down & left"},
	{"H      run left"},
	{"J      run down"},
	{"K      run up"},
	{"L      run right"},
	{"N      run down & right"},
	{"U      run up & right"},
	{"Y      run up & left"},
	{"W      wear armor"},
	{"T      take armor off"},
	{"P      put on ring"},
	{"Q      quit"},
	{"R      remove ring"},
	{"S      save game"},
	{"^      identify trap"},
	{"?      help"},
	{"/      key"},
	{"+      throw"},
	{"-      zap"},
	{"Ctrl t terse message format"},
	{"Ctrl r repeat message"},
	{"Del    search for something hidden"},
	{"Ins    <dir> find something"},
	{"a      repeat command"},
	{"c      rename something"},
	{"i      inventory"},
	{"v      version number"},
	{"D      list what has been discovered"}
}};

const std::array<HelpLine, 23> helpobjs = {{
	{FLOOR,   "the floor"},
	{PLAYER,  "the hero"},
	{FOOD,    "some food"},
	{AMULET,  "the amulet of yendor"},
	{SCROLL,  "a scroll"},
	{WEAPON,  "a weapon"},
	{ARMOR,   "a piece of armor"},
	{GOLD,    "some gold"},
	{STICK,   "a magic staff"},
	{POTION,  "a potion"},
	{RING,    "a magic ring"},
	{0xB2,    "a passage"},  // not PASSAGE (0xB1)
	/* make sure in 40 or 80 column none of line draw set connects */
	/* this is currently in column 1 for 80 */
	{DOOR,    "a door"},
	{ULWALL,  "an upper left corner"},
	{TRAP,    "a trap"},
	{HWALL,   "a horizontal wall"},
	{LRWALL,  "a lower right corner"},
	{LLWALL,  "a lower left corner"},
	{VWALL,   "a vertical wall"},
	{URWALL,  "an upper right corner"},
	{STAIRS,  "a stair case"},
	{MAGIC, ',', BMAGIC, "safe and perilous magic"},
	{'A', '-', 'Z', "26 different monsters"}
}};

/*
 * help:
 *	Print out help screens
 */
void
help(std::span<const HelpLine> lines)
{
	int hrow, hcol;
	bool isfull;
	unsigned char answer = 0;

	ui::display().open_page();
	for (int hcount = 0; hcount < std::ssize(lines) && answer != ESCAPE; hcount++)
	{
		const HelpLine &line = lines[hcount];
		bool last = hcount + 1 == std::ssize(lines);

		isfull = false;
		if ((hcount % (game().options.terse?23:46)) == 0)
			ui::display().clear_page();
		/*
		 * determine row and column
		 */
		hcol = 0;
		if (game().options.terse)
		{
			hrow = hcount % 23;
			if (hrow == 22)
				isfull = true;
		}
		else
		{
			hrow = (hcount % 46) / 2;
			if (hcount % 2)
				hcol = 40;
			if (hrow == 22 && hcol == 40)
				 isfull = true;
		}

		ui::display().write_at(hrow, hcol, line.glyphs());
		ui::display().write(line.h_desc);

		/*
		 * decide if we need print a continue type message
		 */
		if (last || isfull)
		{
			if (last)
				ui::display().write_at(24, 0, "--press space to continue--");
			else if (game().options.terse)
				ui::display().write_at(24, 0, "--Space for more, Esc to continue--");
			else
				ui::display().write_at(24, 0, "--Press space for more, Esc to continue--");
			do
				answer = readchar();
			while (answer != ' ' && answer != ESCAPE) ;
		}
	}
	ui::display().close_page();
}

}  // namespace rogue
