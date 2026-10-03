#include "../support/ScriptedGame.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <utility>

#include "core/Coord.hpp"
#include "core/Glyphs.hpp"
#include "core/Random.hpp"
#include "entities/Creature.hpp"
#include "entities/MonsterCatalog.hpp"
#include "game/Game.hpp"
#include "game/Pool.hpp"
#include "persistence/HighScores.hpp"
#include "persistence/SaveGame.hpp"
#include "ui/Display.hpp"
#include "world/Level.hpp"

/*
 * Whole games played through command() on the headless display: what the
 * replays of tools/replay/ check on the real terminal, kept small enough
 * for the unit tests.
 */

namespace rogue {

namespace {

class Play : public test::ScriptedGame {
protected:
	void TearDown() override
	{
		std::remove(file.c_str());
		ScriptedGame::TearDown();
	}

	// Enough hit points that the keys of a test can't kill him, which would
	// end the program
	static void strengthen()
	{
		Player &player = game().player;
		player.body.stats.s_hpt = player.body.stats.s_maxhp = 30000;
	}

	// Where on the map glyph is, if anywhere
	static std::optional<Coord> find(unsigned char glyph)
	{
		for (int y = 1; y < maxrow; y++)
			for (int x = 0; x < MAXCOLS; x++)
				if (game().level.at({x, y}) == glyph)
					return Coord{x, y};
		return std::nullopt;
	}

	// The keys of a random game, as tools/replay/replay.py types them, but
	// for '>', which needs the stairs here
	static std::string random_keys(unsigned seed, int count)
	{
		constexpr std::string_view letters = "abcdefghijklmn";
		constexpr std::string_view dirs = "hjklyubn";
		std::mt19937 r(seed * 7919);
		auto pick = [&r](std::string_view from) { return from[r() % from.size()]; };
		std::string out;
		while (static_cast<int>(out.size()) < count) {
			unsigned x = r() % 100;
			if (x < 40)
				out += pick(dirs);
			else if (x < 46)
				out += static_cast<char>(pick(dirs) - 'a' + 'A');
			else if (x < 54)
				out += 's';
			else if (x < 72) {
				char cmd = pick("qqrreewWTPRdcD");
				out += cmd;
				if (cmd == 'P' || cmd == 'R')
					out += {pick(letters), pick("lr")};
				else if (cmd == 'c')
					out += {pick(letters), 'x', '\n'};
				else if (cmd != 'D')
					out += pick(letters);
			} else if (x < 80)
				out += {pick("tz"), pick(dirs), pick(letters)};
			else if (x < 83)
				out += 'a';
			else if (x < 85)
				out += {static_cast<char>('2' + r() % 8), pick("shjklyubn")};
			else if (x < 87)
				out += 'i';
			else if (x < 89)
				out += static_cast<char>(ESCAPE);
			else
				out += ' ';
		}
		return out;
	}

	std::string file;
};

}  // namespace

// He takes the stairs down, and each level is made, shown and consistent.
TEST_F(Play, DownTheStairs)
{
	for (Random::Seed seed : {1u, 5u, 42u}) {
		new_game(seed);
		for (int depth = 2; depth <= 8; depth++) {
			std::optional<Coord> stairs = find(STAIRS);
			ASSERT_TRUE(stairs) << "seed " << seed << " depth " << depth;
			game().player.body.pos = *stairs;
			play(">");
			EXPECT_EQ(game().level.depth, depth);
			EXPECT_EQ(game().player.max_level, depth);
			EXPECT_EQ(problems(), "");
			EXPECT_TRUE(row(MAXLINES - 2).starts_with(std::format("Level:{:<4}", depth))) << row(MAXLINES - 2);
			EXPECT_EQ(ui::display().tile_at(game().player.body.pos), PLAYER);
		}
	}
}

// He attacks a dragon with one hit point left, dies, and his score is kept.
TEST_F(Play, FightToTheDeath)
{
	file = (std::filesystem::temp_directory_path() / std::format("rogue-play-{}.scr", ::testing::UnitTest::GetInstance()->random_seed())).string();
	std::ofstream(file).close();		// an empty score file
	new_game(7);
	game().options.score_file = file;
	game().options.name = "Tester";
	game().player.purse = 100;
	game().player.body.stats.s_hpt = 1;

	// The dragon goes on a free square next to him, and he walks into it
	Coord hero = game().player.body.pos;
	std::optional<Coord> beside;
	char toward = 0;
	for (auto [key, delta] : {std::pair{'l', Coord{1, 0}}, {'h', Coord{-1, 0}}, {'j', Coord{0, 1}}, {'k', Coord{0, -1}}}) {
		Coord at = hero + delta;
		if (!beside && step_ok(game().level.seen_at(at))) {
			beside = at;
			toward = key;
		}
	}
	ASSERT_TRUE(beside);
	Creature &dragon = *new_creature();
	entities::new_monster(dragon, 'D', *beside);
	EXPECT_EXIT(play_to_the_end(std::string(500, toward)), ::testing::ExitedWithCode(0), "");

	auto scores = persistence::load_scores(file);
	ASSERT_TRUE(scores);
	ASSERT_EQ(scores->entries.size(), 1u);
	const persistence::ScoreEntry &his = scores->entries.front();
	EXPECT_EQ(his.name, "Tester");
	EXPECT_EQ(his.gold, 90);		// the undertaker takes a tenth
	EXPECT_EQ(his.depth, 1);
	EXPECT_EQ(his.fate, 'D');
}

// A game saved with S and restored plays on as if it had never been saved
// (tools/replay/resume.py checks the same on the terminal). In odd seeds he
// is hasted, so the restored game must not roll his moves again.
TEST_F(Play, RestoredGamePlaysOnLikeTheUnsavedOne)
{
	file = (std::filesystem::temp_directory_path() / std::format("rogue-play-{}.sav", ::testing::UnitTest::GetInstance()->random_seed())).string();
	for (unsigned seed : {1u, 2u, 3u, 4u, 5u, 6u}) {
		// Space ends a --More--, and Escape a prompt for an item or direction
		const std::string before = random_keys(seed, 200) + " \x1b \x1b \x1b ";
		const std::string after = random_keys(seed + 100, 200);

		auto start = [seed] {
			new_game(seed);
			strengthen();
			if (seed % 2)
				game().player.body.flags.set(CreatureFlag::Hasted);
		};
		start();
		play(before + after);
		const std::string unsaved = save();

		start();
		game().options.save_file = file;
		EXPECT_EXIT(play_to_the_end(before + "S\n"), ::testing::ExitedWithCode(0), "") << "seed " << seed;

		reset();
		game().options.menu = "off";
		persistence::MapView v;
		auto loaded = persistence::read_save(file, game(), v);
		ASSERT_TRUE(loaded) << "seed " << seed << ": " << (loaded ? "" : loaded.error().detail);
		std::remove(file.c_str());
		redraw(v);
		play(after);
		EXPECT_EQ(save(), unsaved) << "seed " << seed;
	}
}

}  // namespace rogue
