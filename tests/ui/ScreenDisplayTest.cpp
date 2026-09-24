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
	EXPECT_EQ(s.at(0, 12).style, rogue::ui::Standout);
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

TEST(ScreenDisplay, TilesAreDrawnAndReadBack)
{
	Screen s;
	ScreenDisplay d(s);
	d.draw_tile({10, 5}, 'K');
	EXPECT_EQ(d.tile_at({10, 5}), 'K');
	EXPECT_EQ(s.at(5, 10).ch, 'K');
	EXPECT_EQ(s.at(5, 10).style, rogue::ui::Plain);
	EXPECT_EQ(d.tile_at({11, 5}), ' ');
}

TEST(ScreenDisplay, TileStylesPickTheAttribute)
{
		using rogue::ui::TileStyle;
	Screen s;
	ScreenDisplay d(s);
	d.draw_tile({1, 2}, 'K', TileStyle::Inverse);
	d.draw_tile({2, 2}, '*', TileStyle::Bolt);
	d.draw_tile({3, 2}, '*', TileStyle::FrostBolt);
	EXPECT_EQ(s.at(2, 1).style, rogue::ui::Standout);
	EXPECT_EQ(s.at(2, 2).style, rogue::ui::Style{rogue::ui::Color::Red});
	EXPECT_EQ(s.at(2, 3).style, rogue::ui::Style{rogue::ui::Color::Blue});
}

TEST(ScreenDisplay, MapGlyphsGetTheirColours)
{
		Screen s;
	ScreenDisplay d(s);
	d.draw_tile({4, 4}, 0xfa); // FLOOR
	d.draw_tile({5, 4}, 0x01, rogue::ui::TileStyle::Inverse); // PLAYER in a passage
	EXPECT_EQ(s.at(4, 4).style, rogue::ui::Style{rogue::ui::Color::LightGreen});
	EXPECT_EQ(s.at(4, 5).style, (rogue::ui::Style{rogue::ui::Color::Yellow, rogue::ui::Color::LightGrey}));
}

TEST(ScreenDisplay, TilesOffTheScreenAreIgnored)
{
	Screen s;
	ScreenDisplay d(s);
	s.set_cursor(3, 3);
	d.draw_tile({-1, 5}, 'X');
	d.draw_tile({5, Screen::Rows}, 'X');
	EXPECT_EQ(d.tile_at({-1, 5}), ' ');
	EXPECT_EQ(s.row(), 3);
	EXPECT_EQ(s.col(), 3);
}

TEST(ScreenDisplay, CountShowsAndClears)
{
	Screen s;
	ScreenDisplay d(s);
	d.draw_count(12);
	EXPECT_EQ(row_text(s, 23, 76, 4), "12  ");
	d.draw_count(0);
	EXPECT_EQ(row_text(s, 23, 76, 4), "    ");
}

TEST(ScreenDisplay, WriteReturnsWhereTextEnded)
{
	Screen s;
	ScreenDisplay d(s);
	rogue::Coord end = d.write_at(3, 75, "abcdefgh");
	EXPECT_EQ(row_text(s, 3, 75, 5), "abcde");
	EXPECT_EQ(row_text(s, 4, 0, 3), "fgh");
	EXPECT_EQ(end, (rogue::Coord{3, 4}));
	end = d.write("!");
	EXPECT_EQ(s.at(4, 3).ch, '!');
	EXPECT_EQ(end, (rogue::Coord{4, 4}));
}

TEST(ScreenDisplay, WriteUsesInkAndRestoresTheAttribute)
{
		using rogue::ui::Ink;
	Screen s;
	ScreenDisplay d(s);
	d.write_at(0, 0, "Y", Ink::Reverse);
	d.write("es", Ink::Normal);
	EXPECT_EQ(s.at(0, 0).style, rogue::ui::Standout);
	EXPECT_EQ(s.at(0, 1).style, rogue::ui::Plain);
	EXPECT_EQ(s.style(), rogue::ui::Plain);
}

TEST(ScreenDisplay, ClearLineBlanksToTheRightEdge)
{
	Screen s;
	ScreenDisplay d(s);
	d.write_at(7, 0, "keep this part");
	d.clear_line(7, 5);
	EXPECT_EQ(row_text(s, 7, 0, 14), "keep          ");
}

TEST(ScreenDisplay, ClosingAPageBringsTheGameViewBack)
{
	Screen s;
	ScreenDisplay d(s);
	d.draw_tile({10, 10}, '@');
	d.open_page();
	d.clear_page();
	d.write_at(0, 0, "a) Some food");
	EXPECT_EQ(d.tile_at({10, 10}), ' ');
	d.close_page();
	EXPECT_EQ(d.tile_at({10, 10}), '@');
	EXPECT_EQ(row_text(s, 0, 0, 4), "    ");
}

TEST(ScreenDisplay, TitleLeavesTheCursorAtTheNamePrompt)
{
	Screen s;
	ScreenDisplay d(s);
	d.draw_title();
	EXPECT_EQ(row_text(s, 23, 2, 14), "Rogue's Name? ");
	EXPECT_EQ(s.row(), 23);
	EXPECT_EQ(s.col(), 16);
	d.end_title();
	EXPECT_EQ(row_text(s, 23, 2, 14), std::string(14, ' '));
	EXPECT_EQ(s.at(22, 0).ch, 0xc8); // LLWALL closes the frame
}

TEST(ScreenDisplay, TombstoneCentresTheEpitaph)
{
	Screen s;
	ScreenDisplay d(s);
	d.draw_tombstone("Tester", "a kestral", 42, 2026);
	EXPECT_EQ(row_text(s, 14, 37, 6), "Tester");
	EXPECT_EQ(row_text(s, 16, 35, 9), "a kestral");
	EXPECT_EQ(row_text(s, 18, 37, 5), "42 Au");
	EXPECT_EQ(row_text(s, 19, 38, 4), "2026");
}

TEST(ScreenDisplay, ScoresListOneLinePerEntry)
{
	using rogue::ui::ScoreLine;
	Screen s;
	ScreenDisplay d(s);
	ScoreLine lines[] = {{500, "Conan", " killed by a bat on level 3"}, {20, "Ada", " quit on level 1"}};
	d.draw_scores(lines, 1);
	EXPECT_EQ(row_text(s, 0, 0, 27), "Guildmaster's Hall Of Fame:");
	EXPECT_EQ(row_text(s, 4, 0, 38), "500   Conan killed by a bat on level 3");
	EXPECT_EQ(row_text(s, 5, 0, 25), "20    Ada quit on level 1");
	EXPECT_EQ(s.at(5, 6).style, s.at(5, 0).style); // the new entry is one colour
	EXPECT_NE(s.at(4, 6).style, s.at(4, 0).style); // others show the name in red
}

TEST(ScreenDisplay, MonochromeDropsColours)
{
	using rogue::ui::Color;
	using rogue::ui::Ink;
	using rogue::ui::Style;
	Screen s;
	ScreenDisplay d(s);
	d.set_monochrome(true);
	d.draw_tile({4, 4}, 0xfa); // FLOOR
	d.write_at(0, 0, "x", Ink::Yellow);
	d.write_at(0, 1, "y", Ink::Reverse);
	d.write_at(0, 2, "z", Ink::Underline);
	EXPECT_EQ(s.at(4, 4).style, rogue::ui::Plain);
	EXPECT_EQ(s.at(0, 0).style, rogue::ui::Plain);
	EXPECT_EQ(s.at(0, 1).style, (Style{Color::DarkGrey, Color::LightGrey}));
	EXPECT_TRUE(s.at(0, 2).style.underline);
}
