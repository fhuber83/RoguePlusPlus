#include <gtest/gtest.h>

#include "glyphs.h"
#include "ui/Input.hpp"
#include "ui/Keys.hpp"

using namespace rogue::ui;

// The special keys are the commands the help screen lists for them.
TEST(Keys, SpecialKeysAreCommands)
{
	EXPECT_EQ(command_char(key::Home), 'y');
	EXPECT_EQ(command_char(key::Up), 'k');
	EXPECT_EQ(command_char(key::PageDown), 'n');
	EXPECT_EQ(command_char(key::Backspace), 'h');
	EXPECT_EQ(command_char(key::Insert), '>');
	EXPECT_EQ(command_char(key::Delete), 's');
	EXPECT_EQ(command_char(key::Enter), '\n');
	EXPECT_EQ(command_char(key::function(1)), '?');
	EXPECT_EQ(command_char(key::function(4)), ctrl('R'));
	EXPECT_EQ(command_char(key::function(9)), ctrl('F'));
	EXPECT_EQ(command_char(key::AltF9), 'F');
}

// Any other key is its own character.
TEST(Keys, OtherKeysAreThemselves)
{
	EXPECT_EQ(command_char('h'), 'h');
	EXPECT_EQ(command_char('Q'), 'Q');
	EXPECT_EQ(command_char(ESCAPE), ESCAPE);
	EXPECT_EQ(command_char(0xb1), 0xb1);
}
