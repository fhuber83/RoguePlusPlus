/*
 * All sorts of miscellaneous routines
 *
 * misc.c	1.4		(A.I. Design)	12/14/84
 */

#include "rogue.h"

/*
 * get_dir:
 *      Set up the direction co_ordinate for use in varios "prefix"
 *	commands
 */
bool
get_dir()
{
	int ch;
	std::optional<Coord> dir;
	rogue::Turn &turn = game().turn;

	if (turn.again)
		return true;
	msg("which direction? ");
	do
		if ((ch = readchar()) == ESCAPE) {
			msg("");
			return false;
		}
	while (!(dir = find_dir(ch)));
	turn.delta = *dir;
	msg("");
	if (game().player.body.t_flags.test(ISHUH) && rnd(5) == 0)
		do {
			turn.delta.y = rnd(3) - 1;
			turn.delta.x = rnd(3) - 1;
		} while (turn.delta.y == 0 && turn.delta.x == 0);
	return true;
}

/*
 * find_dir:
 *	The direction a key stands for, or nullopt if it is none
 */
std::optional<Coord>
find_dir(unsigned char ch)
{
	switch (ch) {
		case 'h': case'H': return Coord{-1,  0};
		case 'j': case'J': return Coord{ 0,  1};
		case 'k': case'K': return Coord{ 0, -1};
		case 'l': case'L': return Coord{ 1,  0};
		case 'y': case'Y': return Coord{-1, -1};
		case 'u': case'U': return Coord{ 1, -1};
		case 'b': case'B': return Coord{-1,  1};
		case 'n': case'N': return Coord{ 1,  1};
		default: return std::nullopt;
	}
}

/*
 * help: prints out help screens
 */
void
help(const struct h_list *helpscr)
{
	int hcount = 0;
	int hrow, hcol;
	int isfull;
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


/*
 * d_level:
 *	He wants to go down a level
 */
void
d_level()
{
	rogue::Player &player = game().player;

	if (game().level.at(player.body.t_pos) != STAIRS)
		msg("I see no way down");
	else {
		game().level.depth++;
		new_level();
	}
}

/*
 * u_level:
 *	He wants to go up a level
 */
void
u_level()
{
	rogue::Player &player = game().player;

	if (game().level.at(player.body.t_pos) == STAIRS)
		if (player.has_amulet) {
			game().level.depth--;
			if (game().level.depth == 0)
				total_winner();
			new_level();
			msg("you feel a wrenching sensation in your gut");
		}
		else
			msg("your way is magically blocked");
	else
		msg("I see no way up");
}

/*
 * call:
 *	Allow a user to call a potion, scroll, or ring something
 */
void
call()
{
	Maybe<Item> obj;
	std::span<std::string> guess;
	std::string_view elsewise;
	std::span<const bool> know;
	rogue::Items &items = game().items;

	obj = get_item("call", ItemFilter::callable());
	/*
	 * Make certain that it is somethings that we want to wear
	 */
	if (!obj)
		return;
	switch (obj->o_type)
	{
	case ItemKind::Ring:
		guess = items.r_guess;
		know = items.r_know;
		elsewise = (!guess[obj->o_which].empty() ?
			guess[obj->o_which] : items.r_stones[obj->which<Ring>()]);
		break;
	case ItemKind::Potion:
		guess = items.p_guess;
		know = items.p_know;
		elsewise = (!guess[obj->o_which].empty() ?
			guess[obj->o_which] : items.p_colors[obj->which<Potion>()]);
		break;
	case ItemKind::Scroll:
		guess = items.s_guess;
		know = items.s_know;
		elsewise = (!guess[obj->o_which].empty() ?
			guess[obj->o_which] : items.s_names[obj->which<Scroll>()]);
		break;
	case ItemKind::Stick:
		guess = items.ws_guess;
		know = items.ws_know;
		elsewise = (!guess[obj->o_which].empty() ?
			guess[obj->o_which] : items.ws_made[obj->which<Stick>()]);
		break;
	default:
		msg("you can't call that anything");
		return;
	}
	if (know[obj->o_which])
	{
		msg("that has already been identified");
		return;
	}
	msg("Was called \"{}\"", elsewise);
	msg("what do you want to call it? ");
	if (auto name = input().read_line(MAXNAME); name && !name->empty())
		guess[obj->o_which] = *name;
	msg("");
}

/*
 * prompt player for definition of macro
 */
void
do_macro(std::string &macro)
{
	msg("F9 was {}, enter new macro: ",macro);
	if (auto line = input().read_line(rogue::Options::macro_length)) {
		macro.clear();
		for (char c : *line)
			if (c != ctrl('F'))
				macro += c;
	}
	msg("");
	flush_type();
}



