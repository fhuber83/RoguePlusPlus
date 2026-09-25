#include <gtest/gtest.h>

#include <string_view>

#include "game/Command.hpp"

using rogue::Command;
using rogue::command_of;

namespace {

constexpr int ctrl(char c) { return c & 037; }

}  // namespace

TEST(Command, DirectionKeysMoveAndRun)
{
	for (char c : std::string_view("hjklyubn"))
	{
		EXPECT_EQ(command_of(c), Command::Move) << c;
		EXPECT_EQ(command_of(c - 'a' + 'A'), Command::Run) << c;
	}
}

TEST(Command, Keys)
{
	EXPECT_EQ(command_of('t'), Command::Throw);
	EXPECT_EQ(command_of('Q'), Command::Quit);
	EXPECT_EQ(command_of('i'), Command::Inventory);
	EXPECT_EQ(command_of('q'), Command::Quaff);
	EXPECT_EQ(command_of('>'), Command::Descend);
	EXPECT_EQ(command_of('.'), Command::Rest);
	EXPECT_EQ(command_of('^'), Command::IdentifyTrap);
	EXPECT_EQ(command_of(ctrl('T')), Command::ToggleBrief);
	EXPECT_EQ(command_of(ctrl('F')), Command::TypeMacro);
	EXPECT_EQ(command_of(ctrl('R')), Command::RepeatMessage);
	EXPECT_EQ(command_of(ctrl('L')), Command::Redraw);
}

// Prefix keys and com_char()'s aliases never reach the table.
TEST(Command, UnboundKeysAreIllegal)
{
	for (int key : {'x', 'f', 'g', 'a', '0', ' ', '+', '-', '\b', '\033', '\0'})
		EXPECT_EQ(command_of(key), Command::Illegal) << key;
}

TEST(Command, TakesTurn)
{
	for (Command c : {Command::Move, Command::Run, Command::Throw, Command::Zap,
			Command::Quaff, Command::Read, Command::Search, Command::Rest, Command::Eat,
			Command::Drop, Command::Wield, Command::Wear, Command::TakeOff,
			Command::PutOnRing, Command::RemoveRing})
		EXPECT_TRUE(rogue::takes_turn(c)) << static_cast<int>(c);
	for (Command c : {Command::Illegal, Command::Quit, Command::Inventory,
			Command::Call, Command::Descend, Command::Ascend, Command::HelpObjects,
			Command::HelpCommands, Command::Discoveries, Command::ToggleBrief,
			Command::Macro, Command::TypeMacro, Command::RepeatMessage,
			Command::Version, Command::Save, Command::IdentifyTrap,
			Command::Options, Command::Redraw})
		EXPECT_FALSE(rogue::takes_turn(c)) << static_cast<int>(c);
}

// The keys the original let a count repeat: moves, runs and q r s z t .
TEST(Command, Repeatable)
{
	for (char c : std::string_view("hjklyubnHJKLYUBNqrszt."))
		EXPECT_TRUE(rogue::repeatable(command_of(c))) << c;
	for (char c : std::string_view("QidewWTPRc<>/?DFvS^o"))
		EXPECT_FALSE(rogue::repeatable(command_of(c))) << c;
	EXPECT_FALSE(rogue::repeatable(Command::Illegal));
}
