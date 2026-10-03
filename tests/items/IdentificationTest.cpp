#include <gtest/gtest.h>

#include <format>
#include <optional>
#include <string>

#include "core/Glyphs.hpp"
#include "core/KindTable.hpp"
#include "core/Text.hpp"
#include "entities/Item.hpp"
#include "game/Endings.hpp"
#include "game/Game.hpp"
#include "game/NewGame.hpp"
#include "game/Pool.hpp"
#include "items/Identification.hpp"
#include "items/ItemCatalog.hpp"
#include "items/Kinds.hpp"
#include "items/effects/Weapon.hpp"
#include "rules/Experience.hpp"

namespace rogue {

namespace {

// Each test starts and ends with an empty game whose item looks are set by hand
class Names : public ::testing::Test {
protected:
	void SetUp() override { reset(); }
	void TearDown() override { reset(); }

	static void reset()
	{
		game().pool = Pool();
		game().player = Player();
		game().items = Items();
		game().options = Options();
		Items &items = game().items;
		for (Potion p : kinds<Potion>())
			items.p_colors[p] = "red";
		for (Ring r : kinds<Ring>())
			items.r_stones[r] = "opal";
		for (Stick w : kinds<Stick>()) {
			items.ws_type[w] = "staff";
			items.ws_made[w] = "oak";
		}
	}

	template <typename E>
	static Item item(ItemKind kind, E which, int count = 1)
	{
		Item obj{};
		obj.o_type = kind;
		obj.set_which(which);
		obj.o_count = count;
		return obj;
	}
};

}  // namespace

TEST_F(Names, Scrolls)
{
	constexpr Scroll scroll = Scroll::MonsterConfusion;
	game().items.s_names[scroll] = "zim zam zoo zar bax";
	Item obj = item(ItemKind::Scroll, scroll);
	EXPECT_EQ(items::inv_name(obj, false), "A scroll titled 'zim zam zoo zar bax'");
	game().options.terse = true;	// brief names cut the title
	EXPECT_EQ(items::inv_name(obj, false), "A scroll titled 'zim zam zoo zar b'");
	obj.o_count = 3;
	game().items.s_guess[scroll] = "boom";
	EXPECT_EQ(items::inv_name(obj, false), "3 scrolls called boom");
	game().items.s_know[scroll] = true;
	EXPECT_EQ(items::inv_name(obj, false), std::format("3 scrolls of {}", game().items.s_magic[scroll].mi_name));
}

TEST_F(Names, Potions)
{
	Item obj = item(ItemKind::Potion, Potion::Poison);
	EXPECT_EQ(items::inv_name(obj, false), "A red potion");
	obj.o_count = 2;
	EXPECT_EQ(items::inv_name(obj, false), "2 red potions");
	game().items.p_know[Potion::Poison] = true;
	EXPECT_EQ(items::inv_name(obj, false), std::format("2 potions of {}(red)", game().items.p_magic[Potion::Poison].mi_name));
}

TEST_F(Names, FoodUsesTheFruit)
{
	Item obj = item(ItemKind::Food, Food::Fruit);
	EXPECT_EQ(items::inv_name(obj, false), "A Slime Mold");
	game().options.fruit = "apple";
	EXPECT_EQ(items::inv_name(obj, false), "An apple");
	obj.o_count = 4;
	EXPECT_EQ(items::inv_name(obj, false), "4 apples");
	obj = item(ItemKind::Food, Food::Ration, 2);
	EXPECT_EQ(items::inv_name(obj, false), "2 rations of food");
}

TEST_F(Names, WeaponsAndArmor)
{
	Item &obj = *new_item();	// in the pool, so it can be wielded
	obj = item(ItemKind::Weapon, WeaponType::Mace);
	obj.o_hplus = 1;
	obj.o_dplus = -2;
	EXPECT_EQ(items::inv_name(obj, false), "A mace");
	obj.o_flags.set(ItemFlag::Known);
	EXPECT_EQ(items::inv_name(obj, false), "A +1,-2 mace");
	game().player.weapon = game().pool.id_of(obj);
	EXPECT_EQ(items::inv_name(obj, false), "A +1,-2 mace (weapon in hand)");

	game().player.weapon = std::nullopt;
	Item armor = item(ItemKind::Armor, ArmorType::RingMail);
	armor.o_ac = items::a_class[ArmorType::RingMail] - 1;	// one better than usual
	EXPECT_EQ(items::inv_name(armor, false), "Ring mail");
	armor.o_flags.set(ItemFlag::Known);
	EXPECT_EQ(items::inv_name(armor, false),
		"+1 ring mail [armor class " + std::to_string(11 - armor.o_ac) + "]");
	game().options.expert = true;
	EXPECT_EQ(items::inv_name(armor, false), "+1 ring mail");
}

// The original wrote an unknown stick's name over "A staff " from the third
// character, so it keeps "A" even before a vowel
TEST_F(Names, UnknownSticksKeepTheirArticle)
{
	Item obj = item(ItemKind::Stick, Stick::Light);
	EXPECT_EQ(items::inv_name(obj, false), "A oak staff");
	game().items.ws_guess[Stick::Light] = "zapper";
	EXPECT_EQ(items::inv_name(obj, false), "A staff called zapper(oak)");
}

TEST_F(Names, RingsAndHands)
{
	Item &obj = *new_item();	// in the pool, so it can be worn
	obj = item(ItemKind::Ring, Ring::Protection);
	obj.o_ac = 2;
	game().player.rings[Hand::Left] = game().pool.id_of(obj);
	EXPECT_EQ(items::inv_name(obj, false), "An opal ring (on left hand)");
	game().items.r_know[Ring::Protection] = true;
	obj.o_flags.set(ItemFlag::Known);
	EXPECT_EQ(items::inv_name(obj, false),
		std::format("A +2 ring of {}(opal) (on left hand)", game().items.r_magic[Ring::Protection].mi_name));
}

// Dropping lowercases a capital first letter, listing capitalises it
TEST_F(Names, DropLowercases)
{
	Item obj = item(ItemKind::Potion, Potion::Confusion);
	EXPECT_EQ(items::inv_name(obj, true), "a red potion");
	obj = item(ItemKind::Armor, ArmorType::Leather);
	EXPECT_EQ(items::inv_name(obj, false), "Leather armor");
	EXPECT_EQ(items::inv_name(obj, true), "leather armor");
}

TEST(Formatting, PlusNumbers)
{
	EXPECT_EQ(items::effects::num(0, 0, ARMOR), "+0");
	EXPECT_EQ(items::effects::num(-3, 0, ARMOR), "-3");
	EXPECT_EQ(items::effects::num(2, -1, WEAPON), "+2,-1");
}

TEST(Formatting, KillNames)
{
	EXPECT_EQ(killname('a', true), "an arrow");
	EXPECT_EQ(killname('s', true), "starvation");
	EXPECT_EQ(killname('B', true), "a bat");
	EXPECT_EQ(killname('B', false), "bat");
	EXPECT_EQ(killname('?', true), "God");
}

TEST(Formatting, ControlCharacters)
{
	EXPECT_EQ(io_unctrl('a'), "a");
	EXPECT_EQ(io_unctrl('\t'), " ");
	EXPECT_EQ(io_unctrl(ctrl('R')), "^R");
	EXPECT_EQ(io_unctrl(0x7f), "\\x7f");
}

TEST(Formatting, ExperienceLevels)
{
	EXPECT_EQ(rules::e_levels[0], 10);
	for (int i = 1; i < 19; i++)
		EXPECT_EQ(rules::e_levels[i], 2 * rules::e_levels[i - 1]);
	EXPECT_EQ(rules::e_levels[19], 0);
}

// A scroll title's syllable draws its last letter first, as the original's
// buffer was filled, so a seed gives the same titles
TEST(ScrollTitles, SyllableDrawsLastLetterFirst)
{
	rng().reseed(4242);
	char last = rchr("bcdfghjklmnpqrstvwxyz");
	char vowel = rchr("aeiou");
	char first = rchr("bcdfghjklmnpqrstvwxyz");
	rng().reseed(4242);
	EXPECT_EQ(getsyl(), std::string({first, vowel, last}));
}

}  // namespace rogue
