#include <gtest/gtest.h>

#include <cctype>

#include "core/Ascii.hpp"

// In 0-127 the helpers agree with <cctype> in the "C" locale, which tests run
// in; everything else (negative chars, bytes of 128 or more, key codes) is
// none of them and converts to itself.
TEST(Ascii, MatchesCctypeForAsciiAndNothingElse)
{
	for (int c = -128; c < 0x200; c++) {
		const bool ascii = c >= 0 && c < 128;
		EXPECT_EQ(rogue::is_alpha(c), ascii && std::isalpha(c)) << c;
		EXPECT_EQ(rogue::is_upper(c), ascii && std::isupper(c)) << c;
		EXPECT_EQ(rogue::is_lower(c), ascii && std::islower(c)) << c;
		EXPECT_EQ(rogue::is_digit(c), ascii && std::isdigit(c)) << c;
		EXPECT_EQ(rogue::is_space(c), ascii && std::isspace(c)) << c;
		EXPECT_EQ(rogue::is_print(c), ascii && std::isprint(c)) << c;
		EXPECT_EQ(rogue::to_upper(c), ascii ? std::toupper(c) : c) << c;
		EXPECT_EQ(rogue::to_lower(c), ascii ? std::tolower(c) : c) << c;
	}
}

TEST(Ascii, WorksAtCompileTime)
{
	static_assert(rogue::to_upper('q') == 'Q' && rogue::to_lower('Q') == 'q');
	static_assert(rogue::is_space('\t') && !rogue::is_print('\x7f'));
}
