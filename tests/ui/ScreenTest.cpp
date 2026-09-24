#include <gtest/gtest.h>

#include <deque>

#include "ui/Screen.hpp"
#include "ui/Terminal.hpp"

using rogue::ui::Cell;
using rogue::ui::Screen;
using rogue::ui::Terminal;
namespace dos = rogue::ui::dos;
namespace key = rogue::ui::key;

namespace {

/// Keeps its own copy of the grid, built only from draw() calls.
class FakeTerminal : public Terminal {
public:
	void draw(int row, int col, const Cell &cell) override { cells[row][col] = cell; draws++; }
	void set_cursor(int row, int col) override { cursor_row = row; cursor_col = col; }
	void show_cursor(bool visible) override { cursor_visible = visible; }
	void flush() override { flushes++; }
	void bell() override { bells++; }
	int read_key(int) override
	{
		if (keys.empty())
			return key::None;
		int k = keys.front();
		keys.pop_front();
		return k;
	}

	Screen::Snapshot cells{};
	int draws = 0, flushes = 0, bells = 0;
	int cursor_row = -1, cursor_col = -1;
	bool cursor_visible = false;
	std::deque<int> keys;
};

} // namespace

TEST(Screen, StartsBlankWithCursorHome)
{
	Screen s;
	EXPECT_EQ(s.row(), 0);
	EXPECT_EQ(s.col(), 0);
	EXPECT_EQ(s.at(12, 40), Cell{});
	EXPECT_EQ(s.at(12, 40).ch, ' ');
	EXPECT_EQ(s.attr(), dos::Normal);
}

TEST(Screen, PutWritesAtCursorAndAdvances)
{
	Screen s;
	s.set_cursor(3, 5);
	s.put(0x01, dos::Yellow);
	EXPECT_EQ(s.at(3, 5), (Cell{0x01, dos::Yellow, false}));
	EXPECT_EQ(s.row(), 3);
	EXPECT_EQ(s.col(), 6);
}

TEST(Screen, PutUsesCurrentAttribute)
{
	Screen s;
	s.set_attr(dos::Standout);
	s.put("ab");
	EXPECT_EQ(s.at(0, 0), (Cell{'a', dos::Standout, false}));
	EXPECT_EQ(s.at(0, 1), (Cell{'b', dos::Standout, false}));
	EXPECT_EQ(s.col(), 2);
}

TEST(Screen, ReadBackIsExact)
{
	// ASCII terminals draw walls and corners alike; the grid still tells them apart.
	Screen s;
	s.set_cursor(1, 1);
	s.put(0xc9); // ULWALL
	s.put(0xcd); // HWALL
	EXPECT_EQ(s.at(1, 1).ch, 0xc9);
	EXPECT_EQ(s.at(1, 2).ch, 0xcd);
}

TEST(Screen, PutWrapsAtEndOfLine)
{
	Screen s;
	s.set_cursor(0, Screen::Cols - 1);
	s.put("xy");
	EXPECT_EQ(s.at(0, Screen::Cols - 1).ch, 'x');
	EXPECT_EQ(s.at(1, 0).ch, 'y');
	EXPECT_EQ(s.row(), 1);
	EXPECT_EQ(s.col(), 1);
}

TEST(Screen, PutStaysInBottomRightCorner)
{
	Screen s;
	s.set_cursor(Screen::Rows - 1, Screen::Cols - 1);
	s.put('z');
	EXPECT_EQ(s.at(Screen::Rows - 1, Screen::Cols - 1).ch, 'z');
	EXPECT_EQ(s.row(), Screen::Rows - 1);
	EXPECT_EQ(s.col(), Screen::Cols - 1);
}

TEST(Screen, NewlineClearsRestOfLineAndMovesDown)
{
	Screen s;
	s.set_cursor(2, 0);
	s.put("abcdef");
	s.set_cursor(2, 3);
	s.put('\n');
	EXPECT_EQ(s.at(2, 2).ch, 'c');
	EXPECT_EQ(s.at(2, 3).ch, ' ');
	EXPECT_EQ(s.at(2, 5).ch, ' ');
	EXPECT_EQ(s.row(), 3);
	EXPECT_EQ(s.col(), 0);
}

TEST(Screen, SetCursorRejectsPositionsOffScreen)
{
	Screen s;
	s.set_cursor(4, 4);
	EXPECT_FALSE(s.set_cursor(Screen::Rows, 0));
	EXPECT_FALSE(s.set_cursor(0, -1));
	EXPECT_EQ(s.row(), 4);
	EXPECT_EQ(s.col(), 4);
}

TEST(Screen, LineDrawsLineCellsAndKeepsCursor)
{
	Screen s;
	s.set_cursor(10, 10);
	s.set_attr(dos::Green);
	s.line(0, Screen::Cols - 3, 0xc4, 10, false); // clipped at the right edge
	s.line(Screen::Rows - 2, 0, 0xb3, 10, true);  // clipped at the bottom
	EXPECT_EQ(s.at(0, Screen::Cols - 1), (Cell{0xc4, dos::Green, true}));
	EXPECT_EQ(s.at(0, Screen::Cols - 4), Cell{});
	EXPECT_EQ(s.at(Screen::Rows - 1, 0), (Cell{0xb3, dos::Green, true}));
	EXPECT_EQ(s.row(), 10);
	EXPECT_EQ(s.col(), 10);
}

TEST(Screen, EraseBlanksAndHomesCursor)
{
	Screen s;
	s.set_cursor(5, 5);
	s.put("hello");
	s.erase();
	EXPECT_EQ(s.at(5, 5), Cell{});
	EXPECT_EQ(s.row(), 0);
	EXPECT_EQ(s.col(), 0);
}

TEST(Screen, EraseToEolKeepsCursor)
{
	Screen s;
	s.put("hello");
	s.set_cursor(0, 2);
	s.erase_to_eol();
	EXPECT_EQ(s.at(0, 1).ch, 'e');
	EXPECT_EQ(s.at(0, 2).ch, ' ');
	EXPECT_EQ(s.col(), 2);
}

TEST(Screen, SnapshotRestoresEverythingButCursor)
{
	Screen s;
	s.put("before");
	Screen::Snapshot shot = s.snapshot();
	s.erase();
	s.set_cursor(7, 7);
	s.restore(shot);
	EXPECT_EQ(s.at(0, 5).ch, 'e');
	EXPECT_EQ(s.row(), 7);
	EXPECT_EQ(s.col(), 7);
}

TEST(Screen, ShowCursorReturnsPreviousState)
{
	Screen s;
	EXPECT_TRUE(s.show_cursor(false));
	EXPECT_FALSE(s.show_cursor(true));
	EXPECT_TRUE(s.show_cursor(true));
}

TEST(Screen, WithoutTerminalThereAreNoKeys)
{
	Screen s;
	EXPECT_EQ(s.read_key(0), key::None);
}

TEST(Screen, ConnectRepaintsTheTerminal)
{
	Screen s;
	s.put("pre");
	s.show_cursor(false);
	FakeTerminal t;
	s.connect(&t);
	EXPECT_EQ(t.draws, Screen::Rows * Screen::Cols);
	EXPECT_EQ(t.cells, s.snapshot());
	EXPECT_FALSE(t.cursor_visible);
}

TEST(Screen, TerminalMirrorsEveryWrite)
{
	Screen s;
	FakeTerminal t;
	s.connect(&t);
	s.set_cursor(3, 3);
	s.put("abc");
	s.line(5, 0, 0xcd, 20, false);
	s.set(9, 9, Cell{'!', dos::Red, false});
	Screen::Snapshot shot = s.snapshot();
	s.erase();
	s.restore_row(shot, 3);
	EXPECT_EQ(t.cells, s.snapshot());
	EXPECT_EQ(t.cells[3][4].ch, 'b');
	EXPECT_EQ(t.cells[5][0].ch, ' ');
}

TEST(Screen, RefreshAndReadKeySyncTheCursor)
{
	Screen s;
	FakeTerminal t;
	s.connect(&t);
	s.set_cursor(4, 9);
	s.refresh();
	EXPECT_EQ(t.flushes, 1);
	EXPECT_EQ(t.cursor_row, 4);
	EXPECT_EQ(t.cursor_col, 9);

	s.set_cursor(6, 1);
	t.keys = {'q', key::Enter};
	EXPECT_EQ(s.read_key(-1), 'q');
	EXPECT_EQ(t.cursor_row, 6);
	EXPECT_EQ(t.cursor_col, 1);
	EXPECT_EQ(s.read_key(-1), key::Enter);
	EXPECT_EQ(s.read_key(-1), key::None);
}

TEST(Screen, DisconnectStopsDrawing)
{
	Screen s;
	FakeTerminal t;
	s.connect(&t);
	int draws = t.draws;
	s.connect(nullptr);
	s.put('x');
	s.bell();
	EXPECT_EQ(t.draws, draws);
	EXPECT_EQ(t.bells, 0);
	EXPECT_EQ(s.at(0, 0).ch, 'x');
}
