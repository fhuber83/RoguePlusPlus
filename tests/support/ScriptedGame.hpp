#pragma once

/*
 * A game played through command(), missile(), ... in a test: a new game on
 * level 1, made the way main() makes one, with the keyboard scripted.
 */

#include <gtest/gtest.h>

#include <deque>
#include <string>

#include "ui/Screen.hpp"
#include "ui/ScreenDisplay.hpp"
#include "ui/Terminal.hpp"
#include "rogue.h"

namespace rogue::test {

// Types the keys it is given, then Space for every --More--
class ScriptedTerminal : public ui::Terminal {
public:
	void draw(int, int, const ui::Cell &) override {}
	void set_cursor(int, int) override {}
	void show_cursor(bool) override {}
	void flush() override {}
	void bell() override {}
	int read_key(int) override
	{
		if (keys.empty())
			return ' ';
		int k = keys.front();
		keys.pop_front();
		return k;
	}

	std::deque<int> keys;
};

class ScriptedGame : public ::testing::Test {
protected:
	void SetUp() override
	{
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
		new_level();
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
		return dynamic_cast<ui::ScreenDisplay &>(display());
	}

	static void reset()
	{
		game().pool = Pool();
		game().level = Level();
		game().player = Player();
		game().items = Items();
		game().scheduler = rules::Scheduler();
		game().turn = Turn();
		game().message = MessageLine();
		game().options = Options();
	}

	static std::string problems()
	{
		std::string all;
		for (const std::string &p : pool_problems(game()))
			all += p + "\n";
		return all;
	}

	ScriptedTerminal terminal;
};

}  // namespace rogue::test
