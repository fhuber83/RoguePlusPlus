#include <gtest/gtest.h>

#include <deque>
#include <string>

#include "ui/Screen.hpp"
#include "ui/ScreenDisplay.hpp"
#include "ui/Terminal.hpp"
#include "rogue.h"

namespace {

// Types the keys it is given, then Space for every --More--
class ScriptedTerminal : public rogue::ui::Terminal {
public:
	void draw(int, int, const rogue::ui::Cell &) override {}
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

rogue::ui::ScreenDisplay &screen_display()
{
	return dynamic_cast<rogue::ui::ScreenDisplay &>(display());
}

// A new game on level 1, made the way main() makes one, with the keyboard scripted
class Missile : public ::testing::Test {
protected:
	void SetUp() override
	{
		screen_display().set_animations(false);
		rogue::ui::screen().connect(terminal);
		reset();
		rogue::rng().reseed(4242);
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
		rogue::ui::screen().connect(std::nullopt);
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

	// A direction from the rogue with floor next to him
	static Coord open_direction()
	{
		Coord hero = game().player.body.t_pos;
		for (int dy = -1; dy <= 1; dy++)
			for (int dx = -1; dx <= 1; dx++) {
				Coord next{hero.x + dx, hero.y + dy};
				if ((dx || dy) && game().level.at(next) == FLOOR && !moat(next.y, next.x))
					return {dx, dy};
			}
		ADD_FAILURE() << "the rogue has no floor next to him";
		return {0, 0};
	}

	// A dagger that cannot miss, first in the pack (letter a): its Id
	struct Dagger {
		ItemId id;
	};
	static Dagger give_dagger()
	{
		Item &dagger = *new_item();
		init_weapon(dagger, WeaponType::Dagger);
		dagger.o_hplus = 100;
		game().player.body.t_pack.push_front(dagger);
		game().player.in_pack++;
		return {*game().pool.id_of(dagger)};
	}

	static bool in_use(ItemId id)
	{
		return static_cast<bool>(game().pool.item(std::optional<ItemId>(id)));
	}

	static std::string problems()
	{
		std::string all;
		for (const std::string &p : rogue::pool_problems(game()))
			all += p + "\n";
		return all;
	}

	ScriptedTerminal terminal;
};

}  // namespace

// The original left a weapon that hit in no list and never freed it, so
// the pool check failed and the game could no longer be saved
TEST_F(Missile, AWeaponThatHitsIsUsedUp)
{
	Coord d = open_direction();
	Coord hero = game().player.body.t_pos;
	Coord target{hero.x + d.x, hero.y + d.y};
	Creature &monster = *new_creature();
	new_monster(monster, 'Z', target);
	Dagger dagger = give_dagger();

	terminal.keys = {'a'};
	missile(d.y, d.x);

	EXPECT_FALSE(in_use(dagger.id));
	EXPECT_EQ(problems(), "");		// what save_game() checks before it saves
}

// One that misses, here a wall, still lands on the floor
TEST_F(Missile, AWeaponThatMissesFalls)
{
	Coord d = open_direction();
	Dagger dagger = give_dagger();

	terminal.keys = {'a'};
	missile(d.y, d.x);

	ASSERT_TRUE(in_use(dagger.id));
	EXPECT_TRUE(game().level.objects.contains(dagger.id));
	EXPECT_EQ(problems(), "");
}
