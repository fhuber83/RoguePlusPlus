#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include "persistence/SaveGame.hpp"
#include "ui/ScreenDisplay.hpp"
#include "rogue.h"

using rogue::persistence::MapView;
using rogue::persistence::SaveError;
using rogue::persistence::format_save;
using rogue::persistence::parse_save;
using rogue::persistence::read_save;
using rogue::persistence::write_save;

namespace {

rogue::ui::ScreenDisplay &screen_display()
{
	return dynamic_cast<rogue::ui::ScreenDisplay &>(display());
}

// Each test starts from a new game, made the way main() makes one
class SaveGame : public ::testing::Test {
protected:
	void SetUp() override
	{
		screen_display().set_animations(false);
	}
	void TearDown() override
	{
		reset();
		screen_display().set_animations(true);
	}

	static void reset()
	{
		game().pool = rogue::Pool();
		game().level = rogue::Level();
		game().player = rogue::Player();
		game().items = rogue::Items();
		game().scheduler = rogue::rules::Scheduler();
		game().turn = rogue::Turn();
		game().message = rogue::MessageLine();
		game().options = rogue::Options();
	}

	static void new_game(rogue::Random::Seed seed, int depth)
	{
		reset();
		rogue::rng().reseed(seed);
		init_player();
		init_things();
		init_names();
		init_colors();
		init_stones();
		init_materials();
		start_daemon(rogue::rules::Event::Doctor);
		fuse(rogue::rules::Event::Swander, 70);
		start_daemon(rogue::rules::Event::Stomach);
		start_daemon(rogue::rules::Event::Runners);
		for (int d = 1; d <= depth; d++) {
			game().level.depth = d;
			new_level();
		}
	}

	// The map as the screen shows it
	static MapView view()
	{
		MapView v;
		for (int r = 0; r < rogue::persistence::map_rows; r++)
			for (int x = 0; x < rogue::persistence::map_cols; x++)
				v[r][x] = {display().tile_at({x, r + 1}), display().tile_style_at({x, r + 1})};
		return v;
	}

	static std::string save()
	{
		return format_save(game(), view());
	}

	// Load text into game(), failing the test if it doesn't load
	static MapView load(const std::string &text)
	{
		MapView v;
		auto loaded = parse_save(text, game(), v);
		EXPECT_TRUE(loaded) << (loaded ? "" : loaded.error().detail);
		return v;
	}

	static SaveError::Kind load_error(const std::string &text)
	{
		MapView v;
		auto loaded = parse_save(text, game(), v);
		EXPECT_FALSE(loaded);
		return loaded ? SaveError::Kind::BadFormat : loaded.error().kind;
	}

	// Some state new games don't have yet
	static void stir()
	{
		rogue::Game &g = game();
		rogue::Player &p = g.player;
		// A ring worn, a guess named, a fuse burning, a macro half typed
		Item *ring = new_item();
		ring->o_type = rogue::ItemKind::Ring;
		ring->o_which = 3;
		ring->o_damage = ring->o_hurldmg = "0d0";
		p.body.t_pack.push_front(ring);
		p.rings[1] = ring;
		strcpy(g.items.p_guess[2], "fizzy");
		g.items.p_know[4] = true;
		fuse(rogue::rules::Event::Unconfuse, 9);
		strcpy(g.options.macro, "sss");
		g.turn.typeahead = g.options.macro + 1;
		g.turn.last_item = p.weapon;
		g.turn.last_item_key = 'a';
		strcpy(g.message.last, "you feel a bite in your leg");
		// Monsters after everything a monster can be after
		Item *floor = g.level.objects.first();
		int n = 0;
		for (Creature *tp : g.level.monsters) {
			switch (n++ % 4) {
			case 0: tp->t_dest = &p.body.t_pos; break;
			case 1: tp->t_dest = &g.level.rooms[0].r_gold; break;
			case 2: tp->t_dest = floor ? &floor->o_pos : nullptr; break;
			case 3: tp->t_stats.s_dmg = p.flytrap_damage; break;
			}
		}
		strcpy(p.flytrap_damage, "3d1");
	}
};

}  // namespace

TEST_F(SaveGame, SaveLoadSaveIsIdentical)
{
	for (rogue::Random::Seed seed : {1u, 5u, 42u}) {
		for (int depth : {1, 4, 13, 26}) {
			new_game(seed, depth);
			stir();
			std::string text = save();
			load(text);
			EXPECT_EQ(save(), text) << "seed " << seed << " depth " << depth;
		}
	}
}

// Playing on from a restored game does what playing on without saving did.
TEST_F(SaveGame, RestoredGamePlaysOnTheSame)
{
	for (rogue::Random::Seed seed : {1u, 5u, 42u, 4242u}) {
		new_game(seed, 12);
		stir();
		std::string text = save();

		std::vector<int> rolls;
		game().level.depth++;
		new_level();
		for (int i = 0; i < 50; i++)
			rolls.push_back(rnd(1000));
		std::string played = save();

		load(text);
		game().level.depth++;
		new_level();
		for (int i = 0; i < 50; i++)
			EXPECT_EQ(rnd(1000), rolls[i]);
		EXPECT_EQ(save(), played) << "seed " << seed;
	}
}

TEST_F(SaveGame, PointersPointIntoTheGame)
{
	new_game(42, 5);
	stir();
	std::string text = save();
	load(text);
	rogue::Game &g = game();
	EXPECT_TRUE(rogue::pool_problems(g).empty());
	EXPECT_TRUE(g.player.body.t_pack.contains(g.player.rings[1]));
	EXPECT_STREQ(g.items.p_guess[2], "fizzy");
	EXPECT_STREQ(g.turn.typeahead, "ss");
	EXPECT_EQ(g.turn.last_item, g.player.weapon);
	EXPECT_EQ(g.scheduler.time_left(rogue::rules::Event::Unconfuse), 9);
	int flytraps = 0;
	for (Creature *tp : g.level.monsters)
		if (tp->t_stats.s_dmg == g.player.flytrap_damage)
			flytraps++;
	EXPECT_GT(flytraps, 0);
}

TEST_F(SaveGame, TheScreenComesBack)
{
	new_game(5, 3);
	MapView before = view();
	before[3][7] = {'K', rogue::ui::TileStyle::Inverse};
	before[21][79] = {0xfa, rogue::ui::TileStyle::FrostBolt};
	std::string text = format_save(game(), before);
	MapView after;
	ASSERT_TRUE(parse_save(text, game(), after));
	EXPECT_EQ(after, before);
}

TEST_F(SaveGame, RejectsWhatIsntASave)
{
	new_game(1, 1);
	std::string text = save();
	EXPECT_EQ(load_error("not json"), SaveError::Kind::BadFormat);
	EXPECT_EQ(load_error(R"({"format": "rogue++ scores", "version": 1})"), SaveError::Kind::BadFormat);

	std::string newer = text;
	newer.replace(newer.find("\"version\": 1"), 12, "\"version\": 2");
	EXPECT_EQ(load_error(newer), SaveError::Kind::WrongVersion);

	std::string missing = text;
	missing.replace(missing.find("\"purse\""), 7, "\"purso\"");
	MapView v;
	auto loaded = parse_save(missing, game(), v);
	ASSERT_FALSE(loaded);
	EXPECT_EQ(loaded.error().kind, SaveError::Kind::BadFormat);
	EXPECT_NE(loaded.error().detail.find("purse"), std::string::npos) << loaded.error().detail;
}

TEST_F(SaveGame, RejectsBrokenReferences)
{
	new_game(1, 2);
	std::string text = save();
	// The first floor object listed twice
	auto objects = text.find("\"objects\": [");
	ASSERT_NE(objects, std::string::npos);
	auto first = text.find_first_of("0123456789", objects);
	std::string slot = text.substr(first, text.find_first_not_of("0123456789", first) - first);
	std::string twice = text;
	twice.insert(objects + 12, slot + ", ");
	EXPECT_EQ(load_error(twice), SaveError::Kind::Inconsistent);

	std::string outside = text;
	outside.replace(outside.find("\"objects\": ["), 12, "\"objects\": [9999, ");
	EXPECT_EQ(load_error(outside), SaveError::Kind::BadFormat);
}

TEST_F(SaveGame, WriteAndRead)
{
	new_game(4242, 2);
	auto path = std::filesystem::temp_directory_path() / "rogue_save_test.sav";
	if (const char *keep = std::getenv("ROGUE_KEEP_SAVE"))
		(void)write_save(keep, game(), view());
	std::string text = save();
	ASSERT_TRUE(write_save(path.c_str(), game(), view()));
	EXPECT_FALSE(std::filesystem::exists(path.string() + ".tmp"));
	MapView v;
	auto loaded = read_save(path.c_str(), game(), v);
	std::filesystem::remove(path);
	ASSERT_TRUE(loaded) << loaded.error().detail;
	EXPECT_EQ(save(), text);

	EXPECT_EQ(read_save("/nonexistent/rogue.sav", game(), v).error().kind, SaveError::Kind::Unreadable);
	EXPECT_FALSE(write_save("/nonexistent/dir/rogue.sav", game(), view()));
}
