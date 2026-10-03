#pragma once

/*
 * The message line, and the prompts that wait for a key: messages are built
 * in game().message (game/MessageLine.hpp), shown through the display, and
 * wait for Space at "More" when a new one would cover one not yet read. The
 * first five functions are game().message's members, for the callers that
 * don't hold the game's message line.
 */

#include <format>
#include <string_view>
#include <utility>

#include "game/Game.hpp"

namespace rogue {

/*
 * show_msg:
 *	Add text to the message and show it (after a More if a message is
 *	up). Empty text clears the line.
 */
void show_msg(std::string_view text);

/*
 * add_msg:
 *	Add text to the message being built, cut to fit the buffer.
 */
void add_msg(std::string_view text);

/*
 * endmsg:
 *	Show the message built so far, giving the rogue a chance to read the
 *	one that is up first.
 */
void endmsg();

/*
 * more:
 *	Show a prompt such as " More " after the message and wait for Space.
 */
void more(std::string_view prompt);

/*
 * putmsg:
 *	Put a message on the line, a line's width at a time, waiting at
 *	" Cont " between them.
 */
void putmsg(std::string_view text);

/*
 * noterse:
 *	The text, or "" when the rogue asked for terse or expert messages.
 */
std::string_view noterse(std::string_view text);

/*
 * wait_for:
 *	Wait until the key ch is typed.
 */
void wait_for(unsigned char ch);

/*
 * wait_msg:
 *	Show "[Press Enter to <msg>]" on the bottom line and wait for Enter.
 */
void wait_msg(std::string_view msg);

/*
 * str_attr:
 *	Write text at the cursor, the character after each '%' in reverse.
 */
void str_attr(std::string_view str);

/*
 * msg(), addmsg(), debug() and ifterse() take std::format strings. An empty
 * msg() clears the line.
 */
template <class... Args>
void
msg(std::format_string<Args...> fmt, Args &&...args)
{
	show_msg(std::format(fmt, std::forward<Args>(args)...));
}

template <class... Args>
void
addmsg(std::format_string<Args...> fmt, Args &&...args)
{
	add_msg(std::format(fmt, std::forward<Args>(args)...));
}

// A message from the consistency checks (rogue::config::debug_checks)
template <class... Args>
void
debug(std::format_string<Args...> fmt, Args &&...args)
{
	show_msg(std::format(fmt, std::forward<Args>(args)...));
}

// The terse text for an expert, the other for everyone else
template <class... Args>
void
ifterse(std::format_string<Args...> tfmt, std::format_string<Args...> fmt, Args &&...args)
{
	msg(game().options.expert ? tfmt : fmt, std::forward<Args>(args)...);
}

}  // namespace rogue
