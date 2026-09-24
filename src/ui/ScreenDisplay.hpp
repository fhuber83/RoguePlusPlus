#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>

#include "ui/Cell.hpp"
#include "ui/Display.hpp"
#include "ui/Screen.hpp"

namespace rogue::ui {

/// Display that draws on a text Screen with PC Rogue's layout: messages on
/// the top line, status on lines 23 and 24, the clock bottom right.
class ScreenDisplay final : public Display {
public:
	explicit ScreenDisplay(Screen &screen) : screen_(screen) {}

	void draw_message(std::string_view text) override;
	void clear_message() override;
	void show_more(std::string_view prompt, int col) override;
	void blink_more() override;
	void hide_more() override;
	void draw_tile(Coord pos, std::uint8_t glyph, TileStyle style = TileStyle::Normal) override;
	std::uint8_t tile_at(Coord pos) const override;
	void draw_status(const Status &status) override;
	void draw_clock(int hour, int minute) override;
	void draw_count(int count) override;
	void open_page() override;
	void close_page() override;
	bool page_open() const override { return page_open_; }
	void clear_page() override;
	Coord write_at(int row, int col, std::string_view text, Ink ink = Ink::Normal) override;
	Coord write(std::string_view text, Ink ink = Ink::Normal) override;
	void clear_line(int row, int col = 0) override;
	bool show_cursor(bool visible) override;
	void draw_title() override;
	void end_title() override;
	void draw_tombstone(std::string_view name, std::string_view killer, int gold, int year) override;
	void draw_scores(std::span<const ScoreLine> lines, int highlight) override;
	void draw_winner(bool brief) override;
	void curtain_down() override;
	void curtain_up() override;
	void wipe() override;
	void flush() override;
	void bell() override;

private:
	void text(std::string_view s);
	void text_at(int row, int col, std::string_view s);
	void draw_prompt();
	void draw_covered();
	void ink(Ink ink);
	void centered(int row, std::string_view s);
	void frame(int top, int left, int bottom, int right, bool single);

	Screen &screen_;

	// What curtain_down() drew, for curtain_up()
	Screen::Snapshot curtain_{};

	// Game view kept while a page is open
	Screen::Snapshot game_view_{};
	bool page_open_ = false;

	// More prompt
	std::string prompt_;
	std::array<Cell, Screen::Cols> covered_cells_{};
	int prompt_col_ = 0;
	bool covering_ = false;     // prompt sits on top of the message
	bool prompt_shown_ = false; // prompt (not the covered text) is visible

	// Last status drawn, to redraw only what changed
	std::optional<int> level_, hp_, hp_max_, gold_, armor_;
	int hunger_ = 0; // the line starts out blank, which is what "fed" shows
	std::optional<unsigned> str_, str_max_;
	std::optional<std::string> rank_;
};

} // namespace rogue::ui
