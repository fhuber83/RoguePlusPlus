/*
 * Reading the options file. Replaces env.c (Jon Lane, 10/31/83), whose
 * character-at-a-time parser this follows rule for rule.
 */

#include <algorithm>
#include <fstream>
#include <iterator>
#include <optional>

#include "rogue.h"
#include "persistence/OptionsFile.hpp"

namespace rogue::persistence {

namespace {

// DOS end-of-file mark, common in text files back then
constexpr char dos_eof = 26;

bool is_blank(char ch)
{
	return is_space(ch);
}

// The next character of the file, or nothing at its end
class Reader {
public:
	explicit Reader(std::string_view text) : text_(text) {}

	std::optional<char> next()
	{
		if (pos_ == text_.size())
			return std::nullopt;
		char ch = text_[pos_++];
		return ch == dos_eof ? '\n' : ch;
	}

private:
	std::string_view text_;
	std::size_t pos_ = 0;
};

// Add ch, unless it and the last character are both blanks
void add_collapsed(std::string &s, char ch)
{
	if (!is_blank(s.back()) || !is_blank(ch))
		s += ch;
}

/*
 * Cut to max, drop a blank at the end, then anything from a NUL on, in
 * the order env.c's buffers did it
 */
void finish(std::string &s, std::size_t max)
{
	if (s.size() > max)
		s.resize(max);
	if (is_blank(s.back()))
		s.pop_back();
	if (std::size_t nul = s.find('\0'); nul != std::string::npos)
		s.resize(nul);
}

}  // namespace

std::expected<std::vector<OptionSetting>, OptionsError>
parse_options(std::string_view text)
{
	std::vector<OptionSetting> settings;
	Reader in(text);

	for (;;) {
		/*
		 * Skip white space; only here does the end of the file (or a
		 * NUL) end it cleanly
		 */
		char ch;
		do {
			auto next = in.next();
			if (!next || *next == '\0')
				return settings;
			ch = *next;
		} while (is_blank(ch));

		if (ch == '#') {
			while (auto next = in.next())
				if (*next == '\n')
					break;
			continue;
		}

		OptionSetting setting;
		setting.label = ch;
		for (;;) {
			auto next = in.next();
			if (!next)
				return std::unexpected(OptionsError::BadFormat);
			if (*next == '=' || *next == '-')
				break;
			add_collapsed(setting.label, *next);
		}
		finish(setting.label, max_option_label);
		for (char &c : setting.label)
			c = to_lower(c);

		do {
			auto next = in.next();
			if (!next)
				return std::unexpected(OptionsError::BadFormat);
			ch = *next;
		} while (is_blank(ch));

		setting.value = ch;
		for (auto next = in.next(); next && *next != '\n'; next = in.next())
			add_collapsed(setting.value, *next);
		finish(setting.value, max_option_value);

		settings.push_back(std::move(setting));
	}
}

bool
apply_option(Options &options, const OptionSetting &setting)
{
	/*
	 * Each option with the longest text it takes, as env.c had them.
	 * The macro takes one character less than its buffer would hold.
	 */
	struct Field {
		std::string_view label;
		std::string &text;
		std::size_t max;
	};
	const Field fields[] = {
		{"name",	options.name,		Options::name_length},
		{"scorefile",	options.score_file,	Options::file_length},
		{"savefile",	options.save_file,	Options::file_length},
		{"macro",	options.macro,		Options::macro_length},
		{"fruit",	options.fruit,		Options::name_length},
		{"drive",	options.drive,		 1},
		{"menu",	options.menu,		 3},
		{"screen",	options.screen,		 7},
	};

	for (const Field &field : fields)
		if (setting.label == field.label) {
			field.text = setting.value.substr(0, field.max);
			return true;
		}
	return false;
}

LoadResult
load_options(const std::string &path, Options &options)
{
	std::ifstream file(path, std::ios::binary);
	if (!file)
		return LoadResult::Missing;
	std::string text{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

	auto settings = parse_options(text);
	if (!settings)
		return LoadResult::BadFormat;
	for (const OptionSetting &setting : *settings)
		apply_option(options, setting);
	return LoadResult::Loaded;
}

}  // namespace rogue::persistence
