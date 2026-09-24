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
	void draw_status(const Status &status) override;
	void draw_clock(int hour, int minute) override;

private:
	void text(std::string_view s);
	void text_at(int row, int col, std::string_view s);
	void draw_prompt();
	void draw_covered();

	Screen &screen_;

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
