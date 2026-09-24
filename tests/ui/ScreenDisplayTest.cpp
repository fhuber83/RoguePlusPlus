#include <gtest/gtest.h>

#include <string>

#include "ui/ScreenDisplay.hpp"

using rogue::ui::Screen;
using rogue::ui::ScreenDisplay;
using rogue::ui::Status;

namespace {

std::string row_text(const Screen &s, int row, int col = 0, int len = Screen::Cols)
{
	std::string out;
	for (int c = col; c < col + len; c++)
		out += static_cast<char>(s.at(row, c).ch);
	return out;
}

Status sample_status()
{
	Status st;
	st.level = 3;
	st.hp = 12;
	st.hp_max = 15;
	st.str = 16;
	st.str_max = 16;
	st.gold = 42;
	st.armor = 5;
	st.rank = "Guild Novice";
	return st;
}

} // namespace

TEST(ScreenDisplay, MessageReplacesTheTopLine)
{
	Screen s;
	ScreenDisplay d(s);
	d.draw_message("a long first message");
	d.draw_message("short");
	EXPECT_EQ(row_text(s, 0, 0, 10), "short     ");
	EXPECT_EQ(s.row(), 0);
	EXPECT_EQ(s.col(), 5);
}

TEST(ScreenDisplay, MessageIsCutAtScreenWidth)
{
	Screen s;
	ScreenDisplay d(s);
	d.draw_message(std::string(100, 'x'));
	EXPECT_EQ(row_text(s, 0), std::string(Screen::Cols, 'x'));
	EXPECT_EQ(s.at(1, 0).ch, ' ');
}

TEST(ScreenDisplay, ClearMessageBlanksTheLine)
{
	Screen s;
	ScreenDisplay d(s);
	d.draw_message("hello");
	s.set_cursor(10, 10);
	d.clear_message();
	EXPECT_EQ(row_text(s, 0, 0, 5), "     ");
	EXPECT_EQ(s.row(), 0);
	EXPECT_EQ(s.col(), 0);
}

TEST(ScreenDisplay, MoreGoesAfterTheMessageAndLeavesNoTrace)
{
	Screen s;
	ScreenDisplay d(s);
	d.draw_message("You hit it.");
	d.show_more(" More ", 11);
	EXPECT_EQ(row_text(s, 0, 0, 17), "You hit it. More ");
	EXPECT_EQ(s.at(0, 12).attr, rogue::ui::dos::Standout);
	d.blink_more(); // does nothing when the prompt fits
	EXPECT_EQ(row_text(s, 0, 11, 6), " More ");
	d.hide_more();
	EXPECT_EQ(row_text(s, 0, 0, 17), "You hit it.      ");
}

TEST(ScreenDisplay, MoreCoversTheEndOfAFullLineAndBlinks)
{
	Screen s;
	ScreenDisplay d(s);
	std::string line(Screen::Cols, 'x');
	line.replace(Screen::Cols - 6, 6, "abcdef");
	d.draw_message(line);
	d.show_more(" Cont ", Screen::Cols + 10);
	EXPECT_EQ(row_text(s, 0, Screen::Cols - 6, 6), " Cont ");
	d.blink_more();
	EXPECT_EQ(row_text(s, 0, Screen::Cols - 6, 6), "abcdef");
	d.blink_more();
	EXPECT_EQ(row_text(s, 0, Screen::Cols - 6, 6), " Cont ");
	d.hide_more();
	EXPECT_EQ(row_text(s, 0), line);
}

TEST(ScreenDisplay, StatusLayout)
{
	Screen s;
	ScreenDisplay d(s);
	s.set_cursor(5, 7);
	d.draw_status(sample_status());
	EXPECT_EQ(row_text(s, 23, 0, 74),
	          "Level:3     Hits:12(15)   Str:16(16)    Gold:42     Armor:5   Guild Novice");
	EXPECT_EQ(s.row(), 5);
	EXPECT_EQ(s.col(), 7);
}

TEST(ScreenDisplay, StatusRedrawsOnlyWhatChanged)
{
	Screen s;
	ScreenDisplay d(s);
	Status st = sample_status();
	d.draw_status(st);
	s.set_cursor(23, 0);
	s.put("######"); // scribble over "Level:"
	st.gold = 50;
	d.draw_status(st);
	EXPECT_EQ(row_text(s, 23, 0, 6), "######");
	EXPECT_EQ(row_text(s, 23, 40, 10), "Gold:50   ");
}

TEST(ScreenDisplay, StatusRedrawsWhenOnlyTheMaximumChanges)
{
	Screen s;
	ScreenDisplay d(s);
	Status st = sample_status();
	d.draw_status(st);
	st.hp_max = 20;
	d.draw_status(st);
	EXPECT_EQ(row_text(s, 23, 12, 11), "Hits:12(20)");
}

TEST(ScreenDisplay, HungerShowsAndClears)
{
	Screen s;
	ScreenDisplay d(s);
	Status st = sample_status();
	st.hunger = 2;
	d.draw_status(st);
	EXPECT_EQ(row_text(s, 24, 58, 6), "Weak  ");
	st.hunger = 0;
	d.draw_status(st);
	EXPECT_EQ(row_text(s, 24, 58, 6), "      ");
}

TEST(ScreenDisplay, ClockBottomRightKeepsCursor)
{
	Screen s;
	ScreenDisplay d(s);
	s.set_cursor(3, 4);
	d.draw_clock(9, 5);
	EXPECT_EQ(row_text(s, 24, 75, 5), " 9:05");
	d.draw_clock(12, 30);
	EXPECT_EQ(row_text(s, 24, 75, 5), "12:30");
	EXPECT_EQ(s.row(), 3);
	EXPECT_EQ(s.col(), 4);
}
