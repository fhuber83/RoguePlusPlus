/*
 * The help screens.
 *
 * help() comes from misc.c.
 *
 * misc.c	1.4		(A.I. Design)	12/14/84
 */

#include "rogue.h"

namespace rogue {

/*
 * help:
 *	Print out help screens
 */
void
help(const h_list *helpscr)
{
	int hcount = 0;
	int hrow, hcol;
	bool isfull;
	unsigned char answer = 0;

	display().open_page();
	while (!helpscr->h_desc.empty() && answer != ESCAPE)
	{
		isfull = false;
		if ((hcount % (game().options.terse?23:46)) == 0)
			display().clear_page();
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

		display().write_at(hrow, hcol, helpscr->glyphs());
		display().write(helpscr->h_desc);
		helpscr++;

		/*
		 * decide if we need print a continue type message
		 */
		if (helpscr->h_desc.empty() || isfull)
		{
			if (helpscr->h_desc.empty())
				display().write_at(24, 0, "--press space to continue--");
			else if (game().options.terse)
				display().write_at(24, 0, "--Space for more, Esc to continue--");
			else
				display().write_at(24, 0, "--Press space for more, Esc to continue--");
			do
				answer = readchar();
			while (answer != ' ' && answer != ESCAPE) ;
		}
		hcount++;
	}
	display().close_page();
}

}  // namespace rogue
