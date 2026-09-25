#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <vector>

/*
 * The options file (rogue.opt): lines of "label = value", read at startup
 * into rogue::Options.
 *
 * The format is the one env.c (Jon Lane, 1983) read, quirks included:
 *	- A label runs from the first non-blank character to the first '=' or
 *	  '-'; it may span lines. Labels are compared in lower case.
 *	- The value starts at the next non-blank character, even on a later
 *	  line (so an empty value takes the next line), and runs to the end of
 *	  its line.
 *	- In both, a run of blanks keeps only its first character; then the
 *	  text is cut to its longest length (see below), a blank at the end is
 *	  dropped, and a NUL ends it.
 *	- '#' where a label would start comments out the rest of the line.
 *	- ^Z reads as a newline; a NUL where a label would start ends the file.
 *	- The file may not end inside a label or before a value starts.
 *	- Unknown labels are ignored, and a later line wins over an earlier one.
 */

namespace rogue {

struct Options;

namespace persistence {

struct OptionSetting {
	std::string label;	// lower case
	std::string value;
};

enum class OptionsError {
	BadFormat,			// The file ends inside a label or before its value
};

/*
 * Longest label and value kept; the rest is cut off. env.c's buffers held
 * 10 and 24 characters, but its bounds check let one more through (writing
 * the NUL past the end), so these are what players got. No label is long
 * enough for the limit to matter.
 */
inline constexpr std::size_t max_option_label = 11;
inline constexpr std::size_t max_option_value = 25;

/*
 * parse_options:
 *	Split the text of an options file into its settings, in file order.
 */
std::expected<std::vector<OptionSetting>, OptionsError> parse_options(std::string_view text);

/*
 * apply_option:
 *	Store a setting in its option, cut to the option's length. Returns
 *	false for an unknown label.
 */
bool apply_option(Options &options, const OptionSetting &setting);

enum class LoadResult {
	Loaded,
	Missing,			// The file could not be opened; options are unchanged
	BadFormat,			// Options are unchanged
};

/*
 * load_options:
 *	Read the options file at path into options.
 */
LoadResult load_options(const char *path, Options &options);

}  // namespace persistence
}  // namespace rogue
