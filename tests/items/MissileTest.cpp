#include "../support/ScriptedGame.hpp"

#include <gtest/gtest.h>

#include <optional>

#include "core/Coord.hpp"
#include "core/Glyphs.hpp"
#include "entities/MonsterCatalog.hpp"
#include "game/Game.hpp"
#include "game/Id.hpp"
#include "game/Pool.hpp"
#include "items/Kinds.hpp"
#include "items/effects/Weapon.hpp"

namespace rogue {

namespace {

class Missile : public test::ScriptedGame {
protected:
	// A direction from the rogue with floor next to him
	static Coord open_direction()
	{
		Coord hero = game().player.body.pos;
		for (int dy = -1; dy <= 1; dy++)
			for (int dx = -1; dx <= 1; dx++) {
				Coord next{hero.x + dx, hero.y + dy};
				if ((dx || dy) && game().level.at(next) == FLOOR && !game().level.monster_at(next))
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
		items::effects::init_weapon(dagger, WeaponType::Dagger);
		dagger.o_hplus = 100;
		game().player.body.pack.push_front(dagger);
		game().player.in_pack++;
		return {*game().pool.id_of(dagger)};
	}

	static bool in_use(ItemId id)
	{
		return static_cast<bool>(game().pool.item(std::optional<ItemId>(id)));
	}
};

}  // namespace

// The original left a weapon that hit in no list and never freed it, so
// the pool check failed and the game could no longer be saved
TEST_F(Missile, AWeaponThatHitsIsUsedUp)
{
	Coord d = open_direction();
	Coord hero = game().player.body.pos;
	Coord target{hero.x + d.x, hero.y + d.y};
	Creature &monster = *new_creature();
	entities::new_monster(monster, 'Z', target);
	Dagger dagger = give_dagger();

	terminal.keys = {'a'};
	items::effects::missile(d.y, d.x);

	EXPECT_FALSE(in_use(dagger.id));
	EXPECT_EQ(problems(), "");		// what save_game() checks before it saves
}

// One that misses, here a wall, still lands on the floor
TEST_F(Missile, AWeaponThatMissesFalls)
{
	Coord d = open_direction();
	Dagger dagger = give_dagger();

	terminal.keys = {'a'};
	items::effects::missile(d.y, d.x);

	ASSERT_TRUE(in_use(dagger.id));
	EXPECT_TRUE(game().level.objects.contains(dagger.id));
	EXPECT_EQ(problems(), "");
}

}  // namespace rogue
