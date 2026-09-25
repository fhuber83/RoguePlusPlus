#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "persistence/OptionsFile.hpp"
#include "rogue.h"

using rogue::persistence::LoadResult;
using rogue::persistence::OptionSetting;
using rogue::persistence::OptionsError;
using rogue::persistence::apply_option;
using rogue::persistence::load_options;
using rogue::persistence::parse_options;

namespace {

using Pairs = std::vector<std::pair<std::string, std::string>>;

Pairs parsed(std::string_view text)
{
	auto settings = parse_options(text);
	EXPECT_TRUE(settings.has_value()) << text;
	Pairs pairs;
	if (settings)
		for (const OptionSetting &s : *settings)
			pairs.emplace_back(s.label, s.value);
	return pairs;
}

bool is_bad_format(std::string_view text)
{
	auto settings = parse_options(text);
	return !settings && settings.error() == OptionsError::BadFormat;
}

// Writes text to a file in the temp directory and removes it again
class TempFile {
public:
	TempFile(const char *name, std::string_view text)
		: path_(std::filesystem::temp_directory_path() / name)
	{
		FILE *f = std::fopen(path_.c_str(), "wb");
		std::fwrite(text.data(), 1, text.size(), f);
		std::fclose(f);
	}
	~TempFile() { std::filesystem::remove(path_); }
	const char *path() const { return path_.c_str(); }

private:
	std::filesystem::path path_;
};

}  // namespace

TEST(OptionsFile, LabelsAndValues)
{
	EXPECT_EQ(parsed("name=Fred\nfruit = Kumquat\n"),
		(Pairs{{"name", "Fred"}, {"fruit", "Kumquat"}}));
}

TEST(OptionsFile, EmptyTextHasNoSettings)
{
	EXPECT_EQ(parsed(""), Pairs{});
	EXPECT_EQ(parsed(" \n\t\n"), Pairs{});
}

TEST(OptionsFile, DashSeparatesToo)
{
	EXPECT_EQ(parsed("name - Fred\n"), (Pairs{{"name", "Fred"}}));
	// So a label can't contain one, but a value can
	EXPECT_EQ(parsed("x-y=z\nfruit=ugli-fruit\n"), (Pairs{{"x", "y=z"}, {"fruit", "ugli-fruit"}}));
}

TEST(OptionsFile, LabelsAreLowerCaseValuesAreNot)
{
	EXPECT_EQ(parsed("NaMe=FRED\n"), (Pairs{{"name", "FRED"}}));
}

TEST(OptionsFile, BlankRunsKeepTheirFirstCharacter)
{
	EXPECT_EQ(parsed("fruit=  slime \t\t mold   \n"), (Pairs{{"fruit", "slime mold"}}));
	EXPECT_EQ(parsed("fruit=a\t  b\n"), (Pairs{{"fruit", "a\tb"}}));
	EXPECT_EQ(parsed("my  label  =x\n"), (Pairs{{"my label", "x"}}));
}

TEST(OptionsFile, CarriageReturnsAreBlanks)
{
	EXPECT_EQ(parsed("name=Fred\r\nfruit=fig\r\n"), (Pairs{{"name", "Fred"}, {"fruit", "fig"}}));
}

TEST(OptionsFile, CommentsOnlyWhereALabelStarts)
{
	EXPECT_EQ(parsed("# a comment = with a separator\n  #another\nname=Fred # not a comment\n#last"),
		(Pairs{{"name", "Fred # not a comment"}}));
}

TEST(OptionsFile, ControlZIsANewline)
{
	EXPECT_EQ(parsed("name=Fred\x1a" "fruit=fig\x1a"), (Pairs{{"name", "Fred"}, {"fruit", "fig"}}));
}

TEST(OptionsFile, LastValueNeedsNoNewline)
{
	EXPECT_EQ(parsed("name=Fred"), (Pairs{{"name", "Fred"}}));
}

// The value starts at the next non-blank character, wherever that is.
TEST(OptionsFile, EmptyValueTakesTheNextLine)
{
	EXPECT_EQ(parsed("name=\nfruit=fig\n"), (Pairs{{"name", "fruit=fig"}}));
}

// A label runs to the separator, across lines.
TEST(OptionsFile, LabelWithoutSeparatorRunsOn)
{
	EXPECT_EQ(parsed("junk\nname=Fred\n"), (Pairs{{"junk\nname", "Fred"}}));
}

TEST(OptionsFile, EndingInsideALabelIsAnError)
{
	EXPECT_TRUE(is_bad_format("name"));
	EXPECT_TRUE(is_bad_format("name=Fred\njunk\n"));
	EXPECT_TRUE(is_bad_format("name="));
	EXPECT_TRUE(is_bad_format("name=  \n "));
}

TEST(OptionsFile, NulWhereALabelStartsEndsTheFile)
{
	using namespace std::string_view_literals;
	EXPECT_EQ(parsed("name=Fred\n\0fruit=fig\n"sv), (Pairs{{"name", "Fred"}}));
	// Elsewhere a NUL ends that label or value, as it did in a C string
	EXPECT_EQ(parsed("name\0junk=Fred\0junk\nfruit=fig\n"sv), (Pairs{{"name", "Fred"}, {"fruit", "fig"}}));
}

TEST(OptionsFile, LongValuesAreCut)
{
	std::string text = "macro=" + std::string(30, 'x') + "\n";
	EXPECT_EQ(parsed(text), (Pairs{{"macro", std::string(rogue::persistence::max_option_value, 'x')}}));
}

// Cut first, then a blank at the cut is dropped
TEST(OptionsFile, BlankAtTheCutIsDropped)
{
	std::string kept(rogue::persistence::max_option_value - 1, 'x');
	EXPECT_EQ(parsed("macro=" + kept + " yz\n"), (Pairs{{"macro", kept}}));
}

TEST(OptionsFile, ApplySetsEachOption)
{
	rogue::Options o;
	EXPECT_TRUE(apply_option(o, {"name", "Fred"}));
	EXPECT_TRUE(apply_option(o, {"fruit", "fig"}));
	EXPECT_TRUE(apply_option(o, {"macro", "ss"}));
	EXPECT_TRUE(apply_option(o, {"scorefile", "a.scr"}));
	EXPECT_TRUE(apply_option(o, {"savefile", "a.sav"}));
	EXPECT_TRUE(apply_option(o, {"drive", "c"}));
	EXPECT_TRUE(apply_option(o, {"menu", "sel"}));
	EXPECT_TRUE(apply_option(o, {"screen", "bw"}));
	EXPECT_STREQ(o.name, "Fred");
	EXPECT_STREQ(o.fruit, "fig");
	EXPECT_STREQ(o.macro, "ss");
	EXPECT_STREQ(o.score_file, "a.scr");
	EXPECT_STREQ(o.save_file, "a.sav");
	EXPECT_STREQ(o.drive, "c");
	EXPECT_STREQ(o.menu, "sel");
	EXPECT_STREQ(o.screen, "bw");
}

TEST(OptionsFile, ApplyIgnoresUnknownLabels)
{
	rogue::Options o;
	EXPECT_FALSE(apply_option(o, {"nam", "Fred"}));
	EXPECT_FALSE(apply_option(o, {"name ", "Fred"}));
	EXPECT_STREQ(o.name, "Rodney");
}

TEST(OptionsFile, ApplyCutsToTheOption)
{
	rogue::Options o;
	apply_option(o, {"fruit", "abcdefghijklmnopqrstuvwx"});
	EXPECT_STREQ(o.fruit, "abcdefghijklmnopqrstuvw");
	apply_option(o, {"scorefile", "abcdefghijklmnopq"});
	EXPECT_STREQ(o.score_file, "abcdefghijklmn");
	apply_option(o, {"menu", "select"});
	EXPECT_STREQ(o.menu, "sel");
	apply_option(o, {"drive", "cd"});
	EXPECT_STREQ(o.drive, "c");
}

TEST(OptionsFile, LoadReadsAFile)
{
	TempFile file("rogue_options_test.opt", "# comment\nname = Optimus\nfruit=Kumquat\nmenu=sel\n"
		"scorefile=my.scr\nfruit_is_not_a_label=x\nfruit=Durian\n");
	rogue::Options o;
	EXPECT_EQ(load_options(file.path(), o), LoadResult::Loaded);
	EXPECT_STREQ(o.name, "Optimus");
	EXPECT_STREQ(o.fruit, "Durian");
	EXPECT_STREQ(o.menu, "sel");
	EXPECT_STREQ(o.score_file, "my.scr");
	EXPECT_STREQ(o.macro, "v");
}

TEST(OptionsFile, LoadMissingFile)
{
	rogue::Options o;
	EXPECT_EQ(load_options("/nonexistent/rogue.opt", o), LoadResult::Missing);
	EXPECT_STREQ(o.name, "Rodney");
}

TEST(OptionsFile, LoadBadFileChangesNothing)
{
	TempFile file("rogue_options_bad.opt", "name=Fred\njunk");
	rogue::Options o;
	EXPECT_EQ(load_options(file.path(), o), LoadResult::BadFormat);
	EXPECT_STREQ(o.name, "Rodney");
}
