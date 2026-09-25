#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include "persistence/HighScores.hpp"

using rogue::persistence::ScoreEntry;
using rogue::persistence::ScoresError;
using rogue::persistence::ScoresFormat;
using rogue::persistence::format_scores;
using rogue::persistence::load_scores;
using rogue::persistence::parse_scores;
using rogue::persistence::save_scores;

namespace {

ScoreEntry entry(std::string name, int gold, int fate = 'K')
{
	ScoreEntry e;
	e.name = std::move(name);
	e.gold = gold;
	e.depth = 4;
	e.experience = 3;
	e.fate = fate;
	return e;
}

// One record as rip.c's struct sc_ent was written to disk
std::string legacy_record(const char *name, int rank, int gold, int fate, int level)
{
	struct {
		char sc_name[38];
		std::int32_t sc_rank, sc_gold, sc_fate, sc_level;
	} r;
	std::memset(&r, 0x5a, sizeof r);	// uninitialized bytes after the name
	std::strcpy(r.sc_name, name);
	r.sc_rank = rank;
	r.sc_gold = gold;
	r.sc_fate = fate;
	r.sc_level = level;
	return std::string(reinterpret_cast<const char *>(&r), sizeof r);
}

std::string json_with(std::string_view entries)
{
	return R"({"format": "rogue++ scores", "version": 1, "scores": [)" + std::string(entries) + "]}";
}

bool is_bad_format(std::string_view bytes)
{
	auto list = parse_scores(bytes);
	return !list && list.error() == ScoresError::BadFormat;
}

std::filesystem::path temp_path(const char *name)
{
	return std::filesystem::temp_directory_path() / name;
}

}  // namespace

TEST(HighScores, EmptyFileIsAnEmptyList)
{
	for (std::string_view bytes : {"", "  \n"}) {
		auto list = parse_scores(bytes);
		ASSERT_TRUE(list);
		EXPECT_TRUE(list->entries.empty());
		EXPECT_EQ(list->format, ScoresFormat::Json);
	}
}

TEST(HighScores, RoundTrip)
{
	std::vector<ScoreEntry> entries = {entry("Rodney", 500), entry("Fred", 20, 1)};
	entries[0].depth = 26;
	entries[0].experience = 21;
	entries[0].cause = "killed by a kestrel";

	auto list = parse_scores(format_scores(entries));
	ASSERT_TRUE(list);
	ASSERT_EQ(list->entries.size(), 2u);
	EXPECT_EQ(list->format, ScoresFormat::Json);
	EXPECT_EQ(list->entries[0].name, "Rodney");
	EXPECT_EQ(list->entries[0].gold, 500);
	EXPECT_EQ(list->entries[0].depth, 26);
	EXPECT_EQ(list->entries[0].experience, 21);
	EXPECT_EQ(list->entries[0].fate, 'K');
	EXPECT_EQ(list->entries[0].cause, "");		// only written
	EXPECT_EQ(list->entries[1].name, "Fred");
	EXPECT_EQ(list->entries[1].fate, 1);
}

TEST(HighScores, WrittenAsReadableJson)
{
	ScoreEntry e = entry("Rodney", 312);
	e.cause = "killed by a kestrel";
	std::string text = format_scores(std::vector{e});
	EXPECT_NE(text.find(R"("format": "rogue++ scores")"), std::string::npos) << text;
	EXPECT_NE(text.find(R"("version": 1)"), std::string::npos) << text;
	EXPECT_NE(text.find(R"("name": "Rodney")"), std::string::npos) << text;
	EXPECT_NE(text.find(R"("cause": "killed by a kestrel")"), std::string::npos) << text;
}

// Names are bytes: each is written as the code point of the same value.
TEST(HighScores, NameBytesSurvive)
{
	std::string name = "J\xf6rg \x01\x80\xff";
	std::string text = format_scores(std::vector{entry(name, 10)});
	EXPECT_NE(text.find("Jörg"), std::string::npos) << text;
	auto list = parse_scores(text);
	ASSERT_TRUE(list);
	EXPECT_EQ(list->entries[0].name, name);
}

TEST(HighScores, NamesBeyondLatin1ReadAsQuestionMarks)
{
	auto list = parse_scores(json_with(R"({"name": "a€b", "gold": 1, "depth": 1, "experience": 1, "fate": 1})"));
	ASSERT_TRUE(list);
	EXPECT_EQ(list->entries[0].name, "a?b");
}

TEST(HighScores, LongNamesAreCut)
{
	auto list = parse_scores(format_scores(std::vector{entry(std::string(50, 'x'), 1)}));
	ASSERT_TRUE(list);
	EXPECT_EQ(list->entries[0].name, std::string(rogue::persistence::max_score_name, 'x'));
}

TEST(HighScores, SortedRichestFirstAndCut)
{
	std::vector<ScoreEntry> entries;
	for (int i = 1; i <= 12; i++)
		entries.push_back(entry("p" + std::to_string(i), i % 2 ? i : 0));
	entries.push_back(entry("tie", 11));
	auto list = parse_scores(format_scores(entries));
	ASSERT_TRUE(list);
	// Without gold: dropped; equals keep their order
	std::vector<std::string> names;
	for (const ScoreEntry &e : list->entries)
		names.push_back(e.name);
	EXPECT_EQ(names, (std::vector<std::string>{"p11", "tie", "p9", "p7", "p5", "p3", "p1"}));

	entries.clear();
	for (int i = 1; i <= 12; i++)
		entries.push_back(entry("p" + std::to_string(i), i));
	list = parse_scores(format_scores(entries));
	ASSERT_TRUE(list);
	ASSERT_EQ(list->entries.size(), rogue::persistence::max_scores);
	EXPECT_EQ(list->entries.front().gold, 12);
	EXPECT_EQ(list->entries.back().gold, 3);
}

TEST(HighScores, BadJson)
{
	EXPECT_TRUE(is_bad_format("{"));
	EXPECT_TRUE(is_bad_format("{}"));
	EXPECT_TRUE(is_bad_format(R"({"format": "other", "version": 1, "scores": []})"));
	EXPECT_TRUE(is_bad_format(R"({"format": "rogue++ scores", "version": 2, "scores": []})"));
	EXPECT_TRUE(is_bad_format(R"({"format": "rogue++ scores", "version": 1, "scores": {}})"));
	EXPECT_TRUE(is_bad_format(json_with(R"({"name": "a", "gold": 1, "depth": 1, "experience": 1})")));
	EXPECT_TRUE(is_bad_format(json_with(R"({"name": 5, "gold": 1, "depth": 1, "experience": 1, "fate": 1})")));
	EXPECT_TRUE(is_bad_format(json_with(R"({"name": "a", "gold": 1.5, "depth": 1, "experience": 1, "fate": 1})")));
	EXPECT_TRUE(is_bad_format(json_with(R"({"name": "a", "gold": 1e20, "depth": 1, "experience": 1, "fate": 1})")));
	EXPECT_TRUE(is_bad_format(json_with(R"({"name": "a", "gold": 99999999999, "depth": 1, "experience": 1, "fate": 1})")));
	// The score list shows the title he_man[experience - 1]
	EXPECT_TRUE(is_bad_format(json_with(R"({"name": "a", "gold": 1, "depth": 1, "experience": 0, "fate": 1})")));
	EXPECT_TRUE(is_bad_format(json_with(R"({"name": "a", "gold": 1, "depth": 1, "experience": 22, "fate": 1})")));
}

TEST(HighScores, ReadsTheOriginalBinaryFormat)
{
	std::string bytes = legacy_record("Rodney", 5, 312, 'K', 6) + legacy_record("Fred", 1, 10, 1, 1);
	auto list = parse_scores(bytes);
	ASSERT_TRUE(list);
	EXPECT_EQ(list->format, ScoresFormat::Legacy);
	ASSERT_EQ(list->entries.size(), 2u);
	EXPECT_EQ(list->entries[0].name, "Rodney");
	EXPECT_EQ(list->entries[0].experience, 5);
	EXPECT_EQ(list->entries[0].gold, 312);
	EXPECT_EQ(list->entries[0].fate, 'K');
	EXPECT_EQ(list->entries[0].depth, 6);
	EXPECT_EQ(list->entries[1].name, "Fred");
}

TEST(HighScores, BadBinary)
{
	std::string record = legacy_record("Rodney", 5, 312, 'K', 6);
	EXPECT_TRUE(is_bad_format(record.substr(0, 55)));
	std::string eleven;
	for (int i = 0; i < 11; i++)
		eleven += record;
	EXPECT_TRUE(is_bad_format(eleven));
	EXPECT_TRUE(is_bad_format(legacy_record("Rodney", 0, 312, 'K', 6)));
	EXPECT_TRUE(is_bad_format("not a score file\n"));
}

TEST(HighScores, SaveAndLoad)
{
	auto path = temp_path("rogue_scores_test.scr");
	std::vector<ScoreEntry> entries = {entry("Rodney", 500)};
	ASSERT_TRUE(save_scores(path.c_str(), entries));
	EXPECT_FALSE(std::filesystem::exists(path.string() + ".tmp"));
	auto list = load_scores(path.c_str());
	std::filesystem::remove(path);
	ASSERT_TRUE(list);
	ASSERT_EQ(list->entries.size(), 1u);
	EXPECT_EQ(list->entries[0].name, "Rodney");
}

TEST(HighScores, LoadMissingFile)
{
	auto list = load_scores("/nonexistent/rogue.scr");
	ASSERT_FALSE(list);
	EXPECT_EQ(list.error(), ScoresError::Unreadable);
}

TEST(HighScores, SaveFailureLeavesNothing)
{
	EXPECT_FALSE(save_scores("/nonexistent/dir/rogue.scr", std::vector{entry("Rodney", 1)}));
}
