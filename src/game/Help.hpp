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
	std::array<char, 5> glyph_text{};	// either (ch) or (ch,sep,ch2) appended with ": "
	std::size_t glyph_length = 0;
	std::string_view desc;

	// A line of text (was H_STR)
	constexpr HelpLine(std::string_view desc) : desc(desc) {}
	// A glyph and what it is (was H_CHSTR)
	constexpr HelpLine(unsigned char ch, std::string_view desc)
		: glyph_text{static_cast<char>(ch), ':', ' '}, glyph_length(3), desc(desc) {}
	// Two glyphs with a separator, "A-Z" (was H_CH2STR)
	constexpr HelpLine(unsigned char first, unsigned char sep, unsigned char last, std::string_view desc)
		: glyph_text{static_cast<char>(first), static_cast<char>(sep), static_cast<char>(last), ':', ' '},
		  glyph_length(5), desc(desc) {}

	// The glyph column, "" for a line of text
	constexpr std::string_view glyphs() const { return {glyph_text.data(), glyph_length}; }
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
