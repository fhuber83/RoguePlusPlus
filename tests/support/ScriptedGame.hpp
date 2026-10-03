#pragma once

/*
 * Games played in a test: a new game made the way main() makes one, on the
 * real ScreenDisplay and ScreenInput, with a terminal that types scripted
 * keys. Tests play it through command() (play()), or call missile(), ...
 * directly, and read the screen back (row()).
 */

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <deque>
#include <optional>
#include <string>
#include <string_view>

#include "game/CommandDispatcher.hpp"
#include "game/Game.hpp"
#include "game/NewGame.hpp"
#include "game/StatusLine.hpp"
#include "persistence/SaveGame.hpp"
#include "rules/Durations.hpp"
#include "rules/Scheduler.hpp"
#include "ui/Cell.hpp"
#include "ui/Display.hpp"
#include "ui/Screen.hpp"
#include "ui/ScreenDisplay.hpp"
#include "ui/Terminal.hpp"
#include "world/LevelGenerator.hpp"

namespace rogue::test {

// Thrown by a ScriptedTerminal that stops when its keys run out
struct OutOfKeys {};

// Types the keys it is given, then Space for every --More-- (and Enter for
// the prompts of the ending, by turns), or stops the game where it waits for
// the next key (throws OutOfKeys)
class ScriptedTerminal : public ui::Terminal {
public:
	void draw(int, int, const ui::Cell &) override {}
	void set_cursor(int, int) override {}
	void show_cursor(bool) override {}
	void flush() override {}
	void bell() override {}
	int read_key(int) override
	{
		if (keys.empty()) {
			if (stop_when_empty)
				throw OutOfKeys{};
			// A game that wants nothing but Space forever is stuck
			if (++spaces > 1000) {
				std::fputs("the game asked for 1000 keys more than it was given:\n", stderr);
				for (int y = 0; y < ui::Screen::Rows; y++) {
					for (int x = 0; x < ui::Screen::Cols; x++) {
						unsigned char ch = ui::screen().at(y, x).ch;
						std::fputc(ch >= ' ' && ch < 0x7f ? ch : '?', stderr);
					}
					std::fputc('\n', stderr);
				}
				std::_Exit(3);
			}
			return spaces % 2 ? ' ' : '\n';
		}
		int k = keys.front();
		keys.pop_front();
		return k;
	}

	std::deque<int> keys;
	bool stop_when_empty = false;
	int spaces = 0;
};

class ScriptedGame : public ::testing::Test {
protected:
	void SetUp() override
	{
		static bool guarded = false;
		if (!guarded) {
			std::atexit(exit_guard);
			guarded = true;
		}
		screen_display().set_animations(false);
		ui::screen().connect(terminal);
		reset();
		rng().reseed(4242);
		init_player();
		init_things();
		init_names();
		init_colors();
		init_stones();
		init_materials();
		game().level.depth = 1;
		world::new_level();
		game().options.menu = "off";	// ask for the letter, no inventory page
	}
	void TearDown() override
	{
		reset();
		ui::screen().connect(std::nullopt);
		screen_display().set_animations(true);
	}

	static ui::ScreenDisplay &screen_display()
	{
		return dynamic_cast<ui::ScreenDisplay &>(ui::display());
	}

	static void reset()
	{
		game().pool = Pool();
		game().level = world::Level();
		game().player = Player();
		game().items = Items();
		game().scheduler = rules::Scheduler();
		game().turn = Turn();
		game().message = MessageLine();
		game().options = Options();
	}

	// A new game from seed, made as main() makes one, on level depth (the
	// levels above made and left, as if he had come down)
	static void new_game(Random::Seed seed, int depth = 1)
	{
		reset();
		rng().reseed(seed);
		init_player();
		init_things();
		init_names();
		init_colors();
		init_stones();
		init_materials();
		for (int d = 1; d <= depth; d++) {
			game().level.depth = d;
			world::new_level();
		}
		rules::start_daemon(rules::Event::Doctor);
		rules::fuse(rules::Event::Swander, rules::wander_time());
		rules::start_daemon(rules::Event::Stomach);
		rules::start_daemon(rules::Event::Runners);
		game().options.menu = "off";
	}

	// Type keys and play the commands they make, as playit() does. It stops
	// where the game waits for the key after them, which may be inside a
	// command, where S would save it.
	void play(std::string_view keys)
	{
		terminal.keys.assign(keys.begin(), keys.end());
		terminal.stop_when_empty = true;
		playing = true;
		try {
			for (;;)
				command();
		} catch (const OutOfKeys &) {
		}
		playing = false;
		terminal.stop_when_empty = false;
	}

	// Type keys and play until the game ends the program, with Space for
	// what it asks after them: run it inside EXPECT_EXIT. A game still
	// asking 1000 Spaces later exits with 3.
	[[noreturn]] void play_to_the_end(std::string_view keys)
	{
		terminal.keys.assign(keys.begin(), keys.end());
		terminal.spaces = 0;
		for (;;)
			command();
	}

	// A row of the screen as text (glyph codes as bytes), without the
	// spaces at its end
	static std::string row(int y)
	{
		std::string text;
		for (int x = 0; x < ui::Screen::Cols; x++)
			text += static_cast<char>(ui::screen().at(y, x).ch);
		return text.substr(0, text.find_last_not_of(' ') + 1);
	}

	// The map as the screen shows it, which a save keeps
	static persistence::MapView view()
	{
		persistence::MapView v;
		for (int r = 0; r < persistence::map_rows; r++)
			for (int x = 0; x < persistence::map_cols; x++)
				v[r][x] = {ui::display().tile_at({x, r + 1}), ui::display().tile_style_at({x, r + 1})};
		return v;
	}

	static std::string save()
	{
		return persistence::format_save(game(), view());
	}

	// Load text into game(), failing the test if it doesn't load
	static persistence::MapView load(const std::string &text)
	{
		persistence::MapView v;
		auto loaded = persistence::parse_save(text, game(), v);
		EXPECT_TRUE(loaded) << (loaded ? "" : loaded.error().detail);
		return v;
	}

	// What restore() does after reading a save: show the map and status, and
	// go on with the command the save was made in
	static void redraw(const persistence::MapView &v)
	{
		for (int r = 0; r < persistence::map_rows; r++)
			for (int x = 0; x < persistence::map_cols; x++)
				ui::display().draw_tile({x, r + 1}, v[r][x].glyph, v[r][x].style);
		status();
		resume_saved_game();
	}

	static std::string problems()
	{
		std::string all;
		for (const std::string &p : pool_problems(game()))
			all += p + "\n";
		return all;
	}

	ScriptedTerminal terminal;

private:
	// A game that ends inside play() (he dies, or a key saves or quits)
	// would end the test program as if every test had passed
	static void exit_guard()
	{
		if (playing) {
			std::fputs("the game ended the program inside play()\n", stderr);
			std::_Exit(EXIT_FAILURE);
		}
	}

	static inline bool playing = false;
};

}  // namespace rogue::test
