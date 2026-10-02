#include <gtest/gtest.h>

#include "core/Text.hpp"

using rogue::vowelstr;

TEST(Text, VowelstrGivesTheRestOfTheArticle)
{
	for (std::string_view word : {"amulet", "emerald", "ice monster", "orc", "umber hulk", "Aquator", "Emu", "Ice", "Orc", "Ur-vile"})
		EXPECT_EQ(vowelstr(word), "n") << word;
	for (std::string_view word : {"bat", "yellow", "Zombie", " orc", "1up", ""})
		EXPECT_EQ(vowelstr(word), "") << word;
	static_assert(vowelstr("apple") == "n" && vowelstr("pear").empty());
}
