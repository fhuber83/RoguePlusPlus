/*
 * Commands and prompts of the rogue that belong to no other module: the
 * direction prompt, the stairs, and defining the F9 macro.
 *
 * From misc.c.
 *
 * misc.c	1.4		(A.I. Design)	12/14/84
 */

#include "rogue.h"

namespace rogue {

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
	if (game().player.body.t_flags.test(CreatureFlag::Confused) && rnd(5) == 0)
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
 * do_macro:
 *	Prompt the player for the definition of the F9 macro
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

}  // namespace rogue
