#pragma once

#include <format>
#include <string>
#include <string_view>

#include "core/Ascii.hpp"

namespace rogue {

/*
 * vowelstr:
 *	"n" if the text starts with a vowel, "" otherwise: the rest of the
 *	article, as in std::format("a{} {}", vowelstr(name), name).
 */
constexpr std::string_view
vowelstr(std::string_view text)
{
	if (text.empty())
		return "";
	switch (to_lower(text.front())) {
	case 'a': case 'e': case 'i': case 'o': case 'u':
		return "n";
	default:
		return "";
	}
}

/*
 * io_unctrl:
 *	A readable version of a character: a blank for white space, ^X for a
 *	control character, \xNN for anything else not printable.
 */
inline std::string
io_unctrl(unsigned char ch)
{
	if (is_space(ch))
		return " ";
	if (!is_print(ch)) {
		if (ch < ' ')
			return std::format("^{}", static_cast<char>(ch + '@'));
		return std::format("\\x{:x}", ch);
	}
	return std::string(1, static_cast<char>(ch));
}

}  // namespace rogue
