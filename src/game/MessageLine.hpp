#pragma once

/*
 * The message line: the message being built, the one shown and the last one
 * kept for ^R, and what shows them. game/Messages has the msg() family that
 * puts messages on it.
 */

#include <string>
#include <string_view>

namespace rogue {

inline constexpr int BUFSIZE = 128;	/* the longest message, with its end */

struct MessageLine {
	std::string text;				/* msgbuf: the message being built, at most BUFSIZE - 1 */
	std::string last;				/* huh: the last message printed */
	int end = 0;					/* mpos: where the shown message ends, 0 if none */
	int next_end = 0;				/* newpos: where the message being built ends */
	bool remember = true;			/* save_msg: keep the message for ^R */

	// Add text to the message and show it (after a More if a message is
	// up). Empty text clears the line. (was msg()'s show_msg())
	void show(std::string_view text);
	// Add text to the message being built, cut to fit the buffer
	// (was add_msg())
	void add(std::string_view text);
	// Show the message built so far, giving the rogue a chance to read
	// the one that is up first (was endmsg())
	void end_message();
	// Show a prompt such as " More " after the message and wait for Space
	void more(std::string_view prompt);
	// Put text on the line, a line's width at a time, waiting at " Cont "
	// between them (was putmsg())
	void put(std::string_view text);
};

}  // namespace rogue
