#include "../support/ScriptedGame.hpp"

#include <gtest/gtest.h>

#include <set>

#include "entities/Creature.hpp"
#include "game/CommandDispatcher.hpp"
#include "game/Game.hpp"

namespace rogue {

namespace {

using Haste = test::ScriptedGame;

// The keys one command() reads, each a search, which takes one move
int moves_of_a_command(test::ScriptedTerminal &terminal)
{
	constexpr int typed = 10;
	terminal.keys.assign(typed, 's');
	command();
	return typed - static_cast<int>(terminal.keys.size());
}

}  // namespace

TEST_F(Haste, OneMoveACommand)
{
	for (int i = 0; i < 20; i++)
		EXPECT_EQ(moves_of_a_command(terminal), 1);
	EXPECT_EQ(game().turn.moves_left, 0);
}

// The original never returned from command(), so the haste roll was made
// once and a hasted rogue got one move a command like any other
TEST_F(Haste, TwoOrThreeMovesACommandWhenHasted)
{
	game().player.body.t_flags.set(CreatureFlag::Hasted);
	std::set<int> seen;
	for (int i = 0; i < 20; i++) {
		int moves = moves_of_a_command(terminal);
		EXPECT_TRUE(moves == 2 || moves == 3) << moves;
		seen.insert(moves);
	}
	EXPECT_EQ(seen, (std::set<int>{2, 3}));		// rolled for each command
	EXPECT_EQ(game().turn.moves_left, 0);
}

}  // namespace rogue
