#include <gtest/gtest.h>

#include <string>

#include "rogue.h"

namespace {

// msg(), addmsg() and ifterse() format with std::format into the message line
class Message : public ::testing::Test {
protected:
	void SetUp() override { reset(); }
	void TearDown() override { reset(); }

	static void reset()
	{
		game().message = rogue::MessageLine();
		game().options = rogue::Options();
	}

	static std::string text() { return game().message.text; }
};

}  // namespace

TEST_F(Message, AddmsgFormatsOntoTheLine)
{
	addmsg("you hit the {} ", "bat");
	addmsg("{} times{}", 3, '!');
	EXPECT_EQ(text(), "you hit the bat 3 times!");
	EXPECT_EQ(game().message.next_end, static_cast<int>(text().size()));
}

TEST_F(Message, CharactersAndPercentSigns)
{
	unsigned char letter = 'c';
	addmsg("({:c}) {:d} {}%", letter, letter, 50);
	EXPECT_EQ(text(), "(c) 99 50%");
}

TEST_F(Message, AddmsgIsCutToTheBuffer)
{
	addmsg("{}", std::string(BUFSIZE + 20, 'x'));
	EXPECT_EQ(text(), std::string(BUFSIZE - 1, 'x'));
	addmsg("more");
	EXPECT_EQ(text(), std::string(BUFSIZE - 1, 'x'));
}

TEST_F(Message, MsgShowsTheLineCapitalised)
{
	msg("the {} misses", "bat");
	EXPECT_EQ(text(), "The bat misses");
	EXPECT_EQ(game().message.end, static_cast<int>(text().size()));
	EXPECT_EQ(std::string(game().message.last), "the bat misses");
}

TEST_F(Message, EmptyMsgClearsTheLine)
{
	msg("the {} misses", "bat");
	msg("");
	EXPECT_EQ(game().message.end, 0);
	msg("the {} misses", "bat");
	msg("{}", "");		// empty text, as descend("") gives
	EXPECT_EQ(game().message.end, 0);
}

TEST_F(Message, IfterseChoosesByExpert)
{
	ifterse("{} glows", "your {} glows for a moment", "mace");
	EXPECT_EQ(text(), "Your mace glows for a moment");
	game().message = rogue::MessageLine();
	game().options.expert = true;
	ifterse("{} glows", "your {} glows for a moment", "mace");
	EXPECT_EQ(text(), "Mace glows");
}
