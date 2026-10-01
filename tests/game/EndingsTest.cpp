#include <gtest/gtest.h>

#include <vector>

#include "rogue.h"

using rogue::persistence::ScoreEntry;

namespace {

ScoreEntry
entry(int gold, const char *name = "x")
{
	ScoreEntry e;
	e.name = name;
	e.gold = gold;
	e.depth = 1;
	e.experience = 1;
	return e;
}

std::vector<int>
golds(const std::vector<ScoreEntry> &scores)
{
	std::vector<int> g;
	for (const ScoreEntry &e : scores)
		g.push_back(e.gold);
	return g;
}

}  // namespace

TEST(Endings, FirstScoreIsFirst)
{
	std::vector<ScoreEntry> scores;
	EXPECT_EQ(rogue::add_score(scores, entry(10)), 1);
	EXPECT_EQ(golds(scores), std::vector<int>{10});
}

// A score goes after those with as much gold: the old ones keep their rank.
TEST(Endings, ScoreGoesAfterEquals)
{
	std::vector<ScoreEntry> scores = {entry(100), entry(50, "a"), entry(50, "b"), entry(10)};
	EXPECT_EQ(rogue::add_score(scores, entry(50, "new")), 4);
	EXPECT_EQ(golds(scores), (std::vector<int>{100, 50, 50, 50, 10}));
	EXPECT_EQ(scores[3].name, "new");
}

TEST(Endings, NoGoldIsNoScore)
{
	std::vector<ScoreEntry> scores = {entry(100)};
	EXPECT_EQ(rogue::add_score(scores, entry(0)), 0);
	EXPECT_EQ(golds(scores), std::vector<int>{100});
	std::vector<ScoreEntry> none;
	EXPECT_EQ(rogue::add_score(none, entry(0)), 0);
	EXPECT_TRUE(none.empty());
}

// The list keeps ten: a score below them all is not kept, one above pushes
// the last out.
TEST(Endings, FullListKeepsTen)
{
	std::vector<ScoreEntry> scores;
	for (int g = 100; g >= 10; g -= 10)
		scores.push_back(entry(g));
	ASSERT_EQ(scores.size(), rogue::persistence::max_scores);

	EXPECT_EQ(rogue::add_score(scores, entry(10)), 0);	// as much as the last
	EXPECT_EQ(rogue::add_score(scores, entry(5)), 0);
	EXPECT_EQ(scores.back().gold, 10);

	EXPECT_EQ(rogue::add_score(scores, entry(55)), 6);
	EXPECT_EQ(golds(scores), (std::vector<int>{100, 90, 80, 70, 60, 55, 50, 40, 30, 20}));
	EXPECT_EQ(rogue::add_score(scores, entry(1000)), 1);
	EXPECT_EQ(scores.size(), rogue::persistence::max_scores);
	EXPECT_EQ(scores.back().gold, 30);
}
