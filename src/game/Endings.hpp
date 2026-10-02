#pragma once

#include <string>
#include <vector>

#include "persistence/HighScores.hpp"

/*
 * The ends of a game: dying, winning, and the score list.
 */

namespace rogue {

/*
 * score:
 *	Put this game's score (amount in gold) in the score file and show the
 *	list. flags is 0 for a death, by monst; 1 for quitting; 2 for a total
 *	winner. With game().noscore (rogue++ -s) only shows the list.
 */
void score(int amount, int flags, char monst);

/*
 * add_score:
 *	Put entry in its place among scores, the best first, and keep the
 *	best max_scores. Returns its rank, from 1, or 0 if it isn't among them
 *	(which a score without gold never is).
 */
int add_score(std::vector<persistence::ScoreEntry> &scores, const persistence::ScoreEntry &entry);

/*
 * death:
 *	The tombstone and the score of a rogue killed by monst (a monster's
 *	letter, or 'a' arrow, 'b' bolt, 'd' dart, 'f' fall, 's' starvation),
 *	then exit.
 */
[[noreturn]] void death(char monst);

/*
 * total_winner:
 *	The rogue came up with the amulet: sell his pack, score, and exit.
 */
[[noreturn]] void total_winner();

/*
 * killname:
 *	What killed the rogue, from its code, with an article if doart.
 */
std::string killname(unsigned char monst, bool doart);

}  // namespace rogue
