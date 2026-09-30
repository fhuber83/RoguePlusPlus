#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <regex>
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
		Item &ring = *new_item();
		ring.o_type = rogue::ItemKind::Ring;
		ring.set_which(Ring::Searching);
		ring.o_damage = ring.o_hurldmg = "0d0";
		p.body.t_pack.push_front(ring);
		p.rings[Hand::Right] = g.pool.id_of(ring);
		g.items.p_guess[Potion::Poison] = "fizzy";
		g.items.p_know[Potion::SeeInvisible] = true;
		fuse(rogue::rules::Event::Unconfuse, 9);
		g.options.macro = "sss";
		g.turn.typeahead = "ss";
		g.turn.last_item = p.weapon;
		g.turn.last_item_key = 'a';
		g.turn.moves_left = 2;	// saved in the second of a hasted rogue's three moves
		g.message.last = "you feel a bite in your leg";
		// Monsters after everything a monster can be after
		rogue::Maybe<Item> floor = g.level.objects.first();
		int n = 0;
		for (Creature &tp : g.level.monsters) {
			switch (n++ % 4) {
			case 0: tp.t_dest = Hero{}; break;
			case 1: tp.t_dest = Gold{RoomRef::room(0)}; break;
			case 2:
				if (floor)
					tp.t_dest = *g.pool.id_of(*floor);
				else
					tp.t_dest = std::nullopt;
				break;
			case 3: tp.t_type = 'F'; break;	// a venus flytrap, whose attack grows
			}
		}
		p.fung_hit = 3;
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
	ASSERT_TRUE(g.player.ring_item(Hand::Right));
	EXPECT_TRUE(g.player.body.t_pack.contains(*g.player.ring_item(Hand::Right)));
	EXPECT_EQ(g.items.p_guess[Potion::Poison], "fizzy");
	EXPECT_EQ(g.turn.typeahead, "ss");
	EXPECT_EQ(g.turn.last_item, g.player.weapon);
	EXPECT_EQ(g.scheduler.time_left(rogue::rules::Event::Unconfuse), 9);
	int flytraps = 0;
	for (const Creature &tp : g.level.monsters)
		if (tp.t_type == 'F')
			flytraps++;
	EXPECT_GT(flytraps, 0);
	EXPECT_EQ(g.player.fung_hit, 3);
	EXPECT_EQ(flytrap_attacks(g.player.fung_hit), rogue::Attacks("3d1"));
}

// Rooms and passages come back as the same RoomRef
TEST_F(SaveGame, RoomLinksComeBack)
{
	new_game(42, 5);
	rogue::Game &g = game();
	rogue::Maybe<Creature> tp = g.level.monsters.first();
	ASSERT_TRUE(tp);
	tp->t_room = RoomRef::passage(MAXPASS - 1);
	g.player.old_room = RoomRef::room(MAXROOMS - 1);
	std::optional<RoomRef> here = g.player.body.t_room;
	int slot = g.pool.id_of(*tp)->slot;
	load(save());
	EXPECT_EQ(g.pool.creatures.at(slot)->t_room, RoomRef::passage(MAXPASS - 1));
	EXPECT_EQ(g.player.old_room, RoomRef::room(MAXROOMS - 1));
	EXPECT_EQ(g.player.body.t_room, here);
	g.player.old_room = std::nullopt;
	load(save());
	EXPECT_EQ(g.player.old_room, std::nullopt);
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

	// A room or passage past the last one
	std::smatch room;
	ASSERT_TRUE(std::regex_search(text, room, std::regex(R"re("old_room": \{\s*"(room|passage)": \d+\s*\})re")));
	for (std::string_view bad : {R"("old_room": {"room": 9})", R"("old_room": {"passage": 13})"}) {
		std::string no_room = text;
		no_room.replace(room.position(0), room.length(0), bad);
		EXPECT_EQ(load_error(no_room), SaveError::Kind::BadFormat) << bad;
	}
}

// The moves a hasted rogue has left in the command the save was made in
TEST_F(SaveGame, MovesLeftComeBack)
{
	new_game(42, 1);
	stir();
	const std::string text = save();
	load(text);
	EXPECT_EQ(game().turn.moves_left, 2);

	// A save made before F.2 has none, and goes on with one move
	std::string before = std::regex_replace(text, std::regex(R"re("moves_left": \d+,\s*)re"), "");
	ASSERT_NE(before, text);
	load(before);
	EXPECT_EQ(game().turn.moves_left, 1);

	std::string four = std::regex_replace(text, std::regex(R"re("moves_left": \d+)re"), R"("moves_left": 4)");
	EXPECT_EQ(load_error(four), SaveError::Kind::BadFormat);
}

TEST_F(SaveGame, RejectsDamageThatIsntDamage)
{
	new_game(42, 5);
	stir();
	const std::string text = save();
	auto changed = [&](const std::string &from, const std::string &to) {
		std::string t = text;
		auto at = t.find(from);
		EXPECT_NE(at, std::string::npos) << from;
		return at == std::string::npos ? t : t.replace(at, from.size(), to);
	};
	// A flytrap's damage is the alias, and only a flytrap's
	std::string not_alias = text;
	auto alias = not_alias.find(R"("alias": "flytrap")");
	ASSERT_NE(alias, std::string::npos);
	auto open = not_alias.rfind('{', alias), close = not_alias.find('}', alias);
	not_alias.replace(open, close + 1 - open, R"("1d1")");
	EXPECT_EQ(load_error(not_alias), SaveError::Kind::BadFormat);
	EXPECT_EQ(load_error(changed(R"("damage": "1d4")", R"("damage": {"alias": "flytrap"})")), SaveError::Kind::BadFormat);
	// The flytraps' attack follows from their hits
	EXPECT_EQ(load_error(changed(R"("flytrap_damage": "3d1")", R"("flytrap_damage": "4d1")")), SaveError::Kind::BadFormat);
	EXPECT_EQ(load_error(changed(R"("damage": "0d0")", R"("damage": "1x1")")), SaveError::Kind::BadFormat);
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
