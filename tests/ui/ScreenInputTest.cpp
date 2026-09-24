#include <gtest/gtest.h>

#include <deque>
#include <string>

#include "ui/ScreenInput.hpp"
#include "ui/Terminal.hpp"

using rogue::ui::Cell;
using rogue::ui::Screen;
using rogue::ui::ScreenInput;
namespace key = rogue::ui::key;

namespace {

class ScriptedTerminal : public rogue::ui::Terminal {
public:
	void draw(int, int, const Cell &) override {}
	void set_cursor(int, int) override {}
	void show_cursor(bool) override {}
	void flush() override {}
	void bell() override { bells++; }
	int read_key(int) override
	{
		if (keys.empty())
			return '\n'; // never block a test
		int k = keys.front();
		keys.pop_front();
		return k;
	}

	std::deque<int> keys;
	int bells = 0;
};

struct Fixture {
	Fixture(std::initializer_list<int> keys)
	{
		terminal.keys = keys;
		screen.connect(&terminal);
		screen.set_cursor(23, 16);
	}
	std::string typed(int len) const
	{
		std::string out;
		for (int c = 16; c < 16 + len; c++)
			out += static_cast<char>(screen.at(23, c).ch);
		return out;
	}

	ScriptedTerminal terminal;
	Screen screen;
};

} // namespace

TEST(ScreenInput, ReadsAndEchoesALine)
{
	Fixture f{'B', 'o', 'b', '\n'};
	ScreenInput in(f.screen);
	char buf[24];
	EXPECT_EQ(in.read_line(buf, 23), '\n');
	EXPECT_STREQ(buf, "Bob");
	EXPECT_EQ(f.typed(4), "Bob ");
}

TEST(ScreenInput, BackspaceRemovesTheLastCharacter)
{
	Fixture f{'a', 'b', key::Backspace, 'c', '\b', 'd', key::Enter};
	ScreenInput in(f.screen);
	char buf[24];
	EXPECT_EQ(in.read_line(buf, 23), key::Enter);
	EXPECT_STREQ(buf, "ad");
	EXPECT_EQ(f.typed(3), "ad ");
}

TEST(ScreenInput, EscapeAbandonsTheLine)
{
	Fixture f{'x', 'y', 27};
	ScreenInput in(f.screen);
	char buf[24];
	EXPECT_EQ(in.read_line(buf, 23), 27);
	EXPECT_EQ(buf[0], 27);
	EXPECT_EQ(buf[1], '\0');
	EXPECT_EQ(f.typed(2), "  ");
}

TEST(ScreenInput, StopsAtTheSizeLimitWithABell)
{
	Fixture f{'1', '2', '3', '\n'};
	ScreenInput in(f.screen);
	char buf[8];
	in.read_line(buf, 2);
	EXPECT_STREQ(buf, "12");
	EXPECT_EQ(f.terminal.bells, 1);
}

TEST(ScreenInput, IgnoresKeysThatAreNotText)
{
	Fixture f{key::Up, '\t', 'q', '\n'};
	ScreenInput in(f.screen);
	char buf[8];
	in.read_line(buf, 7);
	EXPECT_STREQ(buf, "q");
}
