/*
 * Various installation dependent routines
 *
 * mach_dep.c	1.4 (A.I. Design) 12/1/84
 */

#include	"rogue.h"


unsigned char swap_bits(
	unsigned char data,
	unsigned i,      // positions of bit sequences to swap
	unsigned j,
	unsigned length  // number of consecutive bits in each sequence
)
{
	unsigned char x = ((data >> i) ^ (data >> j)) & ((1U << length) - 1);
	return data ^ ((x << i) | (x << j));
}


/*
 * setup:
 *	Get starting setup for all games
 */
void
setup()
{
	game().options.terse = FALSE;
	if (COLS == 40)
		game().options.terse = TRUE;
	game().options.expert = game().options.terse;
}


/*@
 * start_terminal:
 *	Start the terminal, or exit with the reason it could not.
 */
void
start_terminal()
{
	if (auto started = rogue::ui::start_terminal(game().options.monochrome); !started)
		fatal("%s", started.error().c_str());
}

/*@
 * Return Epoch time as an integer, with second resolution
 * Simple wrapper to <time.h> time()
 */
long
md_time(void)
{
	return (long)time(NULL);
}


/*@
 * Return current local time as a pointer to a struct
 */
TM *
md_localtime()
{
	static TM md_local;
	time_t secs = time(NULL);
	struct tm *local = localtime(&secs);
	md_local.second = local->tm_sec;
	md_local.minute = local->tm_min;
	md_local.hour   = local->tm_hour;
	md_local.day    = local->tm_mday;
	md_local.month  = local->tm_mon;
	md_local.year   = local->tm_year + 1900;
	return &md_local;
}


/*@
 * Sleep for nanoseconds
 */
void
md_nanosleep(long nanoseconds)
{
	struct timespec ts = {0, nanoseconds};
	nanosleep(&ts, NULL);
}


/*
 * flush_type:
 *	Flush typebuf for traps, etc.
 */
void
flush_type()
{
	game().turn.typeahead = nullstr;
}

/*@
 * I wonder why this is here instead of main.c (or *anywhere* else)
 * Granted, the staff and companies to credit vary by platform, but still...
 */
void
credits()
{

	char tname[25];

	display().draw_title();
	input().read_line(tname,23);
	if (*tname && *tname != ESCAPE)
		strcpy(game().options.name, tname);
	display().end_title();
}


/*
 * Table for IBM extended key translation
 * @ moved from mach_dep.c to curses.c and back: keys as rogue::ui::key values
 */
static const struct xlate {
	int keycode;
	unsigned char keyis;
} xtab[] = {
	{rogue::ui::key::Enter,	'\n'}, //@ Keypad Enter
	{rogue::ui::key::Home,	'y'},
	{rogue::ui::key::Up,	'k'},
	{rogue::ui::key::PageUp,	'u'},
	{rogue::ui::key::Backspace, 'h'},
	{rogue::ui::key::Left,	'h'},
	{rogue::ui::key::Right,	'l'},
	{rogue::ui::key::End,	'b'},
	{rogue::ui::key::Down,	'j'},
	{rogue::ui::key::PageDown,	'n'},
	{rogue::ui::key::Insert,	'>'},
	{rogue::ui::key::Delete,	's'},
	{rogue::ui::key::function(1),	'?'},
	{rogue::ui::key::function(2),	'/'},
	{rogue::ui::key::function(3),	'a'},
	{rogue::ui::key::function(4),	CTRL('R')},
	{rogue::ui::key::function(5),	'c'},
	{rogue::ui::key::function(6),	'D'},
	{rogue::ui::key::function(7),	'i'},
	{rogue::ui::key::function(8),	'^'},
	{rogue::ui::key::function(9),	CTRL('F')},
	{rogue::ui::key::AltF9,	'F'}  //@ ALT+F9
};

/*@
 * Map a key to an 8-bit command character using the translation table
 */
static unsigned char
xlate_ch(int ch)
{
	for (const struct xlate *x = xtab; x < xtab + (sizeof xtab) / sizeof *xtab; x++)
	{
		if (ch == x->keycode)
			return x->keyis;
	}
	return (unsigned char)ch;
}

/*
 * readchar:
 *	Return the next input character, from the macro or from the keyboard.
 */
unsigned char
readchar()
{
	int xch;
	unsigned char ch;

	if (*game().turn.typeahead) {
		SIG2();
		display().flush();  //@ macros
		return(*game().turn.typeahead++);
	}
	/*
	 * while there are no characters in the type ahead buffer
	 * update the status line at the bottom of the screen
	 */
	do
	{
		SIG2();  /* Rogue spends a lot of time here @ you bet! */
		display().flush();  //@ command input
	}
	while ((xch = input().read_key(250)) == rogue::ui::key::None);
	ch = xlate_ch(xch);
	if (ch == ESCAPE)
		game().turn.count = 0;
	return ch;
}


/*@
 * newmem - memory allocater
 *        - motto: use malloc() like any sane software or die in 1985
 *
 * Clients should call free() for allocated objects
 */
char *
newmem(unsigned int nbytes)
{
	void * newaddr;
	if ((newaddr = (char *) malloc(nbytes)) == NULL)
		fatal("No Memory");
	return (char *)newaddr;
}


/*@
 * Originally the message would never be seen, as it used printw() after an
 * endwin(), and there was no other blocking call after it, so any  messages
 * would be cleared instantly after display.
 */
/*
 *  fatal: exit with a message
 *  @ moved from main.c, changed to use varargs and actually print the message
 */
void
fatal(const char *msg, ...)
{
	va_list argp;

	rogue::ui::stop_terminal();

	va_start(argp, msg);
	vprintf(msg, argp);
	va_end(argp);
	md_exit(EXIT_SUCCESS);
}


/*@
 * The single point of exit for Rogue
 * renamed from exit() to avoid conflict with <stdlib.h>
 * moved from croot.c
 */
void md_exit(int status)
{
	rogue::ui::stop_terminal();
	free_ds();
#ifdef ROGUE_DEBUG
	printf("Exited normally\n");
#endif
	exit(status);
}
