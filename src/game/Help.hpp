#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <string_view>

/*
 * The help screens.
 */

namespace rogue {

/*
 * A line of a help screen (was struct h_list): a description, after a
 * glyph column for the map symbols.
 */
struct HelpLine {
	std::array<char, 5> h_chstr{};	// either (ch) or (ch,sep,ch2) appended with ": "
	std::size_t h_chlen = 0;
	std::string_view h_desc;

	// A line of text (was H_STR)
	constexpr HelpLine(std::string_view desc) : h_desc(desc) {}
	// A glyph and what it is (was H_CHSTR)
	constexpr HelpLine(unsigned char ch, std::string_view desc)
		: h_chstr{static_cast<char>(ch), ':', ' '}, h_chlen(3), h_desc(desc) {}
	// Two glyphs with a separator, "A-Z" (was H_CH2STR)
	constexpr HelpLine(unsigned char first, unsigned char sep, unsigned char last, std::string_view desc)
		: h_chstr{static_cast<char>(first), static_cast<char>(sep), static_cast<char>(last), ':', ' '},
		  h_chlen(5), h_desc(desc) {}

	// The glyph column, "" for a line of text
	constexpr std::string_view glyphs() const { return {h_chstr.data(), h_chlen}; }
};

// The commands and their keys (F1, ?)
extern const std::array<HelpLine, 62> helpcoms;
// The symbols on the map (F2, /)
extern const std::array<HelpLine, 23> helpobjs;

/*
 * help:
 *	Show a help screen, a page at a time, until its end or Escape.
 */
void help(std::span<const HelpLine> lines);

}  // namespace rogue
