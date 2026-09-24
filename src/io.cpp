/*
 * Various input/output functions
 *
 * io.c		1.4		(A.I. Design) 12/10/84
 */

#include	"ui/Display.hpp"

#include	"rogue.h"

#define AC(a) (-((a)-11))
/*
 * msg:
 *	Display a message at the top of the screen.
 */
static int newpos = 0;

static void more_at(const char *msg, int col);

/* VARARGS1 */
/*@ nope, it was not vargars. But now it is */
void
ifterse(const char *tfmt, const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);

	if (expert)
		vmsg(tfmt, argp);
	else
		vmsg(fmt, argp);

	va_end(argp);
}

//@ va_list variant of msg()
void
vmsg(const char *fmt, va_list argp)
{
	/*
	 * if the string is "", just clear the line
	 */
	if (*fmt == '\0')
	{
		rogue::ui::display().clear_message();
		mpos = 0;
		return;
	}
	/*
	 * otherwise add to the message and flush it out
	 */
	doadd(fmt, argp);
	endmsg();
}

//@ varargs variant, now a wrapper for vmsg()
void
msg(const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);

	vmsg(fmt, argp);

	va_end(argp);
}
/* VARARGS1
 * @ now for real
 */
/*
 * addmsg:
 *	Add things to the current message
 */
void
addmsg(const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);

	doadd(fmt, argp);

	va_end(argp);
}

/*
 * endmsg:
 *	Display a new msg (giving him a chance to see the previous one
 *	if it is up there with the -More-)
 */
void
endmsg(void)
{
	if (save_msg)
		strcpy(huh, msgbuf);
	if (mpos) {
		look(FALSE);
		more_at(" More ", mpos);
	}
	/*
	 * All messages should start with uppercase, except ones that
	 * start with a pack addressing character
	 */
	if (is_lower(msgbuf[0]) && msgbuf[1] != ')')
		msgbuf[0] = toupper(msgbuf[0]);
	putmsg(msgbuf);
	mpos = newpos;
	newpos = 0;
}


/*
 *  More:  tag the end of a line and wait for a space
 *  @ The prompt goes after the current message. Drawing is the display's
 */
void
more(const char *msg)
{
	more_at(msg, mpos);
}

//@ more() for a message line text that ends in column col
static void
more_at(const char *msg, int col)
{
	rogue::ui::Display &display = rogue::ui::display();

	display.show_more(msg, col);
	while (readchar() != ' ')
		display.blink_more();
	display.hide_more();
}


/*@
* arguments changed from fixed ints to va_list.
* no need of a varargs version as this is only used internally by io.c
* varargs-aware functions
*/
/*
 * doadd:
 *	Perform an add onto the message buffer
 */
void
doadd(const char *fmt, va_list argp)
{

	vsnprintf(&msgbuf[newpos], BUFSIZE - newpos, fmt, argp);
	newpos = strlen(msgbuf);
}

/*
 * putmsg:
 *  put a msg on the line, make sure that it will fit, if it won't
 *  scroll msg sideways until he has read it all
 */
void
putmsg(char *msg)
{
	char *curmsg, *lastmsg=0, *tmpmsg;
	int curlen;

	curmsg = msg;
	do {
		rogue::ui::display().draw_message(curmsg);
		newpos = curlen = strlen(curmsg);
		if (curlen > COLS) {
			more_at(" Cont ", curlen);
			lastmsg = curmsg;
			do {
				tmpmsg = strpbrk(curmsg," ");
				/*
				 * If there are no blanks in line
				 */
				if ((tmpmsg==0 || tmpmsg>=&lastmsg[COLS]) && lastmsg==curmsg) {
					curmsg = &lastmsg[COLS];
					break;
				}
				if ((tmpmsg >= (lastmsg+COLS)) || ((signed)strlen(curmsg) < COLS))
					break;
				curmsg = tmpmsg + 1;
			} while (1);
		}
	} while (curlen > COLS);
}

/*
 * io_unctrl:
 *	Print a readable version of a certain character
 *	@ renamed to avoid conflict with <curses.h>
 *	@ same purpose but different behavior, so not using the curses version
 */
char *
io_unctrl(byte ch)
{
	static char chstr[9];		/* Defined in curses library */

	if (is_space(ch))
		strcpy(chstr," ");
	else if (!is_print(ch))
		if (ch < ' ')
			sprintf(chstr, "^%c", ch + '@');
		else
			sprintf(chstr, "\\x%x",ch);
	else {
		chstr[0] = ch;
		chstr[1] = 0;
	}

	return chstr;
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

	SIG2();

	/*@
	 * The armor class shown ignores rings of protection, as it always did
	 */
	ac = cur_armor != NULL ? cur_armor->o_ac : pstats.s_arm;

	st.level = level;
	st.hp = pstats.s_hpt;
	st.hp_max = max_hp;
	st.str = pstats.s_str;
	st.str_max = max_stats.s_str;
	st.gold = purse;
	st.armor = AC(ac);
	st.rank = he_man[pstats.s_lvl-1];
	st.hunger = hungry_state;
	rogue::ui::display().draw_status(st);
}

/*
 * wait_for
 *	Sit around until the guy types the right key
 */
void
wait_for(byte ch)
{
	/*@
	 * stdio and ncurses will map all stream line endings to '\n'
	 * Hooray ANSI! :)
	 *
	char c;

	if (ch == '\n')
		while ((c = readchar()) != '\n' && c != '\r')
			continue;
	else
	 */
	while (readchar() != ch)
		continue;
}

/*@
 * Wait with a message until user press Enter
 * New function, used to block before leaving the game
 */
void
wait_msg(const char *msg)
{
	char prompt[MAXSTR];

	display().show_cursor(TRUE);
	if (*msg)
		snprintf(prompt, sizeof prompt, "[Press Enter to %s]", msg);
	else
		strcpy(prompt, "[Press Enter]");
	display().write_at(LINES-1, 0, prompt);
	flush_type();
	wait_for('\n');
	display().write_at(LINES-1, 0, "");
}

/*
 * show_win:
 *	Function used to display a window and wait before returning
 *	@ a window? looks like a single message to me!
 */
void
show_win(char *message)
{
	display().write_at(0, 0, message);
	display().write_at(hero.y, hero.x, "");
	wait_for(' ');
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
 * key_state:
 *	@ Periodic status update: draws the clock in the bottom-right corner.
 *	@ The original also showed NUM LOCK/CAP LOCK and toggled "Fast Play" via
 *	@ Scroll Lock by reading keyboard LEDs through BIOS; terminals cannot
 *	@ report those, so faststate stays FALSE.
 */
void
SIG2(void)
{
	static int bighand, littlehand;
	static long cur_time = 0;
	int showtime = FALSE;
	long new_time = md_time();

	/*@
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
		showtime = TRUE;
	}

	if (reinit)
	{
		reinit = FALSE;
		showtime = TRUE;
	}

	if (showtime)
		rogue::ui::display().draw_clock(bighand ? bighand : 12, littlehand);
}

const char *
noterse(const char *str)
{
	return( terse || expert ? nullstr : str);
}
