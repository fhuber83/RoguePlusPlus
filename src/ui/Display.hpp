#pragma once

#include <string_view>

namespace rogue::ui {

/// What the status lines at the bottom of the screen show.
struct Status {
	int level = 0;
	int hp = 0;
	int hp_max = 0;
	unsigned str = 0;
	unsigned str_max = 0;
	int gold = 0;
	int armor = 0;         ///< armour class as shown (higher is better)
	std::string_view rank; ///< experience title, "Guild Novice" etc.
	int hunger = 0;        ///< 0 fed, 1 hungry, 2 weak, 3 faint
};

/// How the game shows things to the player. Game logic decides what to
/// show and when to wait for keys; a Display only draws.
class Display {
public:
	virtual ~Display() = default;

	// Message line

	/// Replaces the message line with `text`, cut at the screen width.
	/// Leaves the cursor after the text.
	virtual void draw_message(std::string_view text) = 0;
	/// Blanks the message line and puts the cursor at its start.
	virtual void clear_message() = 0;

	/// Shows `prompt` (" More ", " Cont ") after a message that ends in
	/// column `col`. If it does not fit, it covers the end of the line
	/// instead, and blink_more() alternates it with what it covers.
	virtual void show_more(std::string_view prompt, int col) = 0;
	virtual void blink_more() = 0;
	/// Removes the prompt again.
	virtual void hide_more() = 0;

	// Status lines

	/// Redraws the fields that changed since the last call.
	/// The cursor does not move.
	virtual void draw_status(const Status &status) = 0;
	/// Shows the time (12-hour clock). The cursor does not move.
	virtual void draw_clock(int hour, int minute) = 0;
};

/// The display the game uses.
Display &display();

} // namespace rogue::ui
