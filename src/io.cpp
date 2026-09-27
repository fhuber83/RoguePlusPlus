/*
 * Various input/output functions
 *
 * io.c		1.4		(A.I. Design) 12/10/84
 */

#include	<algorithm>

#include	"ui/Display.hpp"

#include	"rogue.h"

// The armor class the status line shows: the game's counts down from 11 (was AC())
static constexpr int
armor_class(int ac)
{
	return -(ac - 11);
}

/*
 * msg:
 *	Display a message at the top of the screen.
 */

static void more_at(const char *msg, int col);

/*
 * msg(), addmsg() and ifterse() are templates in rogue.h that format with
 * std::format and pass the text on to these.
 */

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
endmsg(void)
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
		message.text[0] = toupper(message.text[0]);
	putmsg(message.text);
	message.end = message.next_end;
	message.next_end = 0;
}


/*
 *  More:  tag the end of a line and wait for a space
 *  The prompt goes after the current message. Drawing is the display's
 */
void
more(const char *msg)
{
	more_at(msg, game().message.end);
}

// more() for a message line text that ends in column col
static void
more_at(const char *msg, int col)
{
	rogue::ui::Display &display = rogue::ui::display();

	display.show_more(msg, col);
	while (readchar() != ' ')
		display.blink_more();
	display.hide_more();
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
 * io_unctrl:
 *	Print a readable version of a certain character
 */
std::string
io_unctrl(unsigned char ch)
{
	if (is_space(ch))
		return " ";
	else if (!is_print(ch))
		if (ch < ' ')
			return std::format("^{}", static_cast<char>(ch + '@'));
		else
			return std::format("\\x{:x}", ch);
	else
		return std::string(1, static_cast<char>(ch));
}

/*
 * status:
 *	Display the important stats line.  Keep the cursor where it was.
 */
void
status(void)
{
	rogue::ui::Status st;
	int ac;
	rogue::Player &player = game().player;

	SIG2();

	/*
	 * The armor class shown ignores rings of protection, as it always did
	 */
	ac = player.armor != nullptr ? player.armor->o_ac : player.body.t_stats.s_arm;

	st.level = game().level.depth;
	st.hp = player.body.t_stats.s_hpt;
	st.hp_max = player.body.t_stats.s_maxhp;
	st.str = player.body.t_stats.s_str;
	st.str_max = player.max_stats.s_str;
	st.gold = player.purse;
	st.armor = armor_class(ac);
	st.rank = he_man[player.body.t_stats.s_lvl-1];
	st.hunger = player.hungry_state;
	rogue::ui::display().draw_status(st);
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
wait_msg(const char *msg)
{
	display().show_cursor(true);
	display().write_at(LINES-1, 0,
		*msg ? std::format("[Press Enter to {}]", msg) : "[Press Enter]");
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
str_attr(const char *str)
{
	while (*str)
	{
		rogue::ui::Ink ink = rogue::ui::Ink::Normal;
		if (*str == '%') {
			str++;
			ink = rogue::ui::Ink::Reverse;
		}
		display().write(std::string_view(str++, 1), ink);
	}
}

/*
 * SIG2:
 *	Periodic status update: draws the clock in the bottom-right corner.
 *	The original also showed NUM LOCK/CAP LOCK and toggled "Fast Play" via
 *	Scroll Lock by reading keyboard LEDs through BIOS; terminals cannot
 *	report those, so faststate stays false.
 */
void
SIG2(void)
{
	static int bighand, littlehand;
	static long cur_time = 0;
	int showtime = false;
	long new_time = md_time();

	/*
	 * Do not update while a page (inventory, discoveries, ...) is shown
	 */
	if (display().page_open())
		return;
	if (new_time - cur_time >= 60)
	{
		TM *local = md_localtime();
		bighand = local->hour % 12;
		littlehand = local->minute;
		cur_time = new_time - local->second;
		showtime = true;
	}

	if (showtime)
		rogue::ui::display().draw_clock(bighand ? bighand : 12, littlehand);
}

const char *
noterse(const char *str)
{
	return( game().options.brief() ? "" : str);
}
