/*
 * The message line, and the prompts that wait for a key.
 *
 * io.c		1.4		(A.I. Design) 12/10/84
 */

#include "rogue.h"

namespace rogue {

namespace {

// more() for a message line text that ends in column col
void
more_at(std::string_view msg, int col)
{
	rogue::ui::Display &display = rogue::ui::display();

	display.show_more(msg, col);
	while (readchar() != ' ')
		display.blink_more();
	display.hide_more();
}

}  // namespace

void
show_msg(std::string_view text)
{
	/*
	 * if the string is "", just clear the line
	 */
	if (text.empty())
	{
		rogue::ui::display().clear_message();
		game().message.end = 0;
		return;
	}
	/*
	 * otherwise add to the message and flush it out
	 */
	add_msg(text);
	endmsg();
}

/*
 * endmsg:
 *	Display a new msg (giving him a chance to see the previous one
 *	if it is up there with the -More-)
 */
void
endmsg()
{
	rogue::MessageLine &message = game().message;
	if (message.remember)
		message.last = message.text;
	if (message.end) {
		look(false);
		more_at(" More ", message.end);
	}
	/*
	 * All messages should start with uppercase, except ones that
	 * start with a pack addressing character
	 */
	if (is_lower(message.text[0]) && message.text[1] != ')')
		message.text[0] = to_upper(message.text[0]);
	putmsg(message.text);
	message.end = message.next_end;
	message.next_end = 0;
}


/*
 *  More:  tag the end of a line and wait for a space
 *  The prompt goes after the current message. Drawing is the display's
 */
void
more(std::string_view msg)
{
	more_at(msg, game().message.end);
}



/*
 * add_msg:
 *	Perform an add onto the message buffer, cut to fit
 */
void
add_msg(std::string_view text)
{
	rogue::MessageLine &message = game().message;
	std::size_t room = BUFSIZE - 1 - message.next_end;

	// Written where the message being built ends; a shown one is kept till then
	message.text.resize(message.next_end);
	message.text += text.substr(0, room);
	// A NUL ended the C string this was
	if (std::size_t nul = message.text.find('\0'); nul != std::string::npos)
		message.text.resize(nul);
	message.next_end = static_cast<int>(message.text.size());
}

/*
 * putmsg:
 *  put a msg on the line, make sure that it will fit, if it won't
 *  scroll msg sideways until he has read it all
 */
void
putmsg(std::string_view msg)
{
	std::string_view cur = msg;		/* what is left to show */
	int curlen;

	do {
		rogue::ui::display().draw_message(cur);
		game().message.next_end = curlen = static_cast<int>(cur.size());
		if (curlen > COLS) {
			more_at(" Cont ", curlen);
			/*
			 * Go on after the last blank that the line showed, or after the
			 * line's width if its first word is longer
			 */
			const std::string_view shown = cur;
			for (;;) {
				std::size_t blank = cur.find(' ');
				std::size_t at = (blank == std::string_view::npos) ? blank
					: static_cast<std::size_t>(cur.data() - shown.data()) + blank;
				/*
				 * If there are no blanks in line
				 */
				if (at >= static_cast<std::size_t>(COLS) && cur.data() == shown.data()) {
					cur = shown.substr(COLS);
					break;
				}
				if (at >= static_cast<std::size_t>(COLS) || cur.size() < static_cast<std::size_t>(COLS))
					break;
				cur = shown.substr(at + 1);
			}
		}
	} while (curlen > COLS);
}

/*
 * wait_for
 *	Sit around until the guy types the right key
 */
void
wait_for(unsigned char ch)
{
	while (readchar() != ch)
		continue;
}

/*
 * wait_msg:
 *	Wait with a message until the user presses Enter
 */
void
wait_msg(std::string_view msg)
{
	display().show_cursor(true);
	display().write_at(LINES-1, 0,
		!msg.empty() ? std::format("[Press Enter to {}]", msg) : "[Press Enter]");
	flush_type();
	wait_for('\n');
	display().write_at(LINES-1, 0, "");
}

/*
 * str_attr:  format a string with attributes.
 *
 *    formats:
 *        %i - the following character is turned inverse vidio
 *        %I - All characters upto %$ or null are turned inverse vidio
 *        %u - the following character is underlined
 *        %U - All characters upto %$ or null are underlined
 *        %$ - Turn off all attributes
 *
 *     Attributes do not nest, therefore turning on an attribute while
 *     a different one is in effect simply changes the attribute.
 *
 *     "No attribute" is the default and is set on leaving this routine
 *
 *     Eventually this routine will contain colors and character intensity
 *     attributes.  And I'm not sure how I'm going to interface this with
 *     printf certainly '%' isn't a good choice of characters.  jll.
 */
void
str_attr(std::string_view str)
{
	for (std::size_t i = 0; i < str.size(); i++)
	{
		rogue::ui::Ink ink = rogue::ui::Ink::Normal;
		if (str[i] == '%') {
			if (++i == str.size())
				break;
			ink = rogue::ui::Ink::Reverse;
		}
		display().write(str.substr(i, 1), ink);
	}
}

/*
 * noterse:
 *	The text, unless messages are brief
 */
std::string_view
noterse(std::string_view str)
{
	return( game().options.brief() ? "" : str);
}

}  // namespace rogue
