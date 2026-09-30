#pragma once

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

}  // namespace rogue
