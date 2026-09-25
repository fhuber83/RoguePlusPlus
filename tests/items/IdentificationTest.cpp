#include <gtest/gtest.h>

#include <string>

#include "rogue.h"

namespace {

// Each test starts and ends with an empty game whose item looks are set by hand
class Names : public ::testing::Test {
protected:
	void SetUp() override { reset(); }
	void TearDown() override { reset(); }

	static void reset()
	{
		game().pool = rogue::Pool();
		game().player = rogue::Player();
		game().items = rogue::Items();
		game().options = rogue::Options();
		rogue::Items &items = game().items;
		for (int i = 0; i < MAXSCROLLS; i++)
			items.s_guess[i] = items.guesses[i].storage;
		for (int i = 0; i < MAXPOTIONS; i++) {
			items.p_guess[i] = items.guesses[MAXSCROLLS + i].storage;
			items.p_colors[i] = "red";
		}
		for (int i = 0; i < MAXRINGS; i++) {
			items.r_guess[i] = items.guesses[MAXSCROLLS + MAXPOTIONS + i].storage;
			items.r_stones[i] = "opal";
		}
		for (int i = 0; i < MAXSTICKS; i++) {
			items.ws_guess[i] = items.guesses[MAXSCROLLS + MAXPOTIONS + MAXRINGS + i].storage;
			items.ws_type[i] = "staff";
			items.ws_made[i] = "oak";
		}
	}

	static Item item(ItemKind kind, int which, int count = 1)
	{
		Item obj{};
		obj.o_type = kind;
		obj.o_which = which;
		obj.o_count = count;
		return obj;
	}
};

}  // namespace

TEST_F(Names, Scrolls)
{
	strcpy(game().items.s_names[0].storage, "zim zam zoo zar bax");
	Item obj = item(ItemKind::Scroll, 0);
	EXPECT_EQ(inv_name(&obj, false), "A scroll titled 'zim zam zoo zar bax'");
	game().options.terse = true;	// brief names cut the title
	EXPECT_EQ(inv_name(&obj, false), "A scroll titled 'zim zam zoo zar b'");
	obj.o_count = 3;
	strcpy(game().items.s_guess[0], "boom");
	EXPECT_EQ(inv_name(&obj, false), "3 scrolls called boom");
	game().items.s_know[0] = true;
	EXPECT_EQ(inv_name(&obj, false), std::string("3 scrolls of ") + game().items.s_magic[0].mi_name);
}

TEST_F(Names, Potions)
{
	Item obj = item(ItemKind::Potion, 2);
	EXPECT_EQ(inv_name(&obj, false), "A red potion");
	obj.o_count = 2;
	EXPECT_EQ(inv_name(&obj, false), "2 red potions");
	game().items.p_know[2] = true;
	EXPECT_EQ(inv_name(&obj, false), std::string("2 potions of ") + game().items.p_magic[2].mi_name + "(red)");
}

TEST_F(Names, FoodUsesTheFruit)
{
	Item obj = item(ItemKind::Food, 1);
	EXPECT_EQ(inv_name(&obj, false), "A Slime Mold");
	strcpy(game().options.fruit, "apple");
	EXPECT_EQ(inv_name(&obj, false), "An apple");
	obj.o_count = 4;
	EXPECT_EQ(inv_name(&obj, false), "4 apples");
	obj = item(ItemKind::Food, 0, 2);
	EXPECT_EQ(inv_name(&obj, false), "2 rations of food");
}

TEST_F(Names, WeaponsAndArmor)
{
	Item obj = item(ItemKind::Weapon, MACE);
	obj.o_hplus = 1;
	obj.o_dplus = -2;
	EXPECT_EQ(inv_name(&obj, false), "A mace");
	obj.o_flags.set(ISKNOW);
	EXPECT_EQ(inv_name(&obj, false), "A +1,-2 mace");
	game().player.weapon = &obj;
	EXPECT_EQ(inv_name(&obj, false), "A +1,-2 mace (weapon in hand)");

	game().player.weapon = nullptr;
	Item armor = item(ItemKind::Armor, RING_MAIL);
	armor.o_ac = a_class[RING_MAIL] - 1;	// one better than usual
	EXPECT_EQ(inv_name(&armor, false), "Ring mail");
	armor.o_flags.set(ISKNOW);
	EXPECT_EQ(inv_name(&armor, false),
		"+1 ring mail [armor class " + std::to_string(11 - armor.o_ac) + "]");
	game().options.expert = true;
	EXPECT_EQ(inv_name(&armor, false), "+1 ring mail");
}

// The original wrote an unknown stick's name over "A staff " from the third
// character, so it keeps "A" even before a vowel
TEST_F(Names, UnknownSticksKeepTheirArticle)
{
	Item obj = item(ItemKind::Stick, 0);
	EXPECT_EQ(inv_name(&obj, false), "A oak staff");
	strcpy(game().items.ws_guess[0], "zapper");
	EXPECT_EQ(inv_name(&obj, false), "A staff called zapper(oak)");
}

TEST_F(Names, RingsAndHands)
{
	Item obj = item(ItemKind::Ring, R_PROTECT);
	obj.o_ac = 2;
	game().player.rings[LEFT] = &obj;
	EXPECT_EQ(inv_name(&obj, false), "An opal ring (on left hand)");
	game().items.r_know[R_PROTECT] = true;
	obj.o_flags.set(ISKNOW);
	EXPECT_EQ(inv_name(&obj, false),
		std::string("A +2 ring of ") + game().items.r_magic[R_PROTECT].mi_name + "(opal) (on left hand)");
}

// Dropping lowercases a capital first letter, listing capitalises it
TEST_F(Names, DropLowercases)
{
	Item obj = item(ItemKind::Potion, 0);
	EXPECT_EQ(inv_name(&obj, true), "a red potion");
	obj = item(ItemKind::Armor, LEATHER);
	EXPECT_EQ(inv_name(&obj, false), "Leather armor");
	EXPECT_EQ(inv_name(&obj, true), "leather armor");
}

TEST(Formatting, PlusNumbers)
{
	EXPECT_EQ(num(0, 0, ARMOR), "+0");
	EXPECT_EQ(num(-3, 0, ARMOR), "-3");
	EXPECT_EQ(num(2, -1, WEAPON), "+2,-1");
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
	EXPECT_EQ(io_unctrl(CTRL('R')), "^R");
	EXPECT_EQ(io_unctrl(0x7f), "\\x7f");
}

TEST(Formatting, ExperienceLevels)
{
	EXPECT_EQ(e_levels[0], 10);
	for (int i = 1; i < 19; i++)
		EXPECT_EQ(e_levels[i], 2 * e_levels[i - 1]);
	EXPECT_EQ(e_levels[19], 0);
}
