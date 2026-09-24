/*
 * Read and execute the user comamnds
 *
 * command.c	1.44	(A.I. Design)	2/14/85
 */

#include	"rogue.h"


void
command()
{
	int ntimes;
	rogue::Player &player = game().player;

	if (on(player.body, ISHASTE))
		ntimes = rnd(2) + 2;
	else
		ntimes = 1;
	while (ntimes--) {
		status();
#ifdef WIZARD
		if (wizard)
			game().noscore = TRUE;
#endif
		if (player.no_command) {
			if (--player.no_command <= 0) {
				msg("you can move again");
				player.no_command = 0;
			}
			display().flush();  //@ sleeping, fainted, frozen, etc
		} else
			execcom();
		do_fuses();
		do_daemons();
		for (ntimes = LEFT; ntimes <= RIGHT; ntimes++)
		{
			if (player.rings[ntimes])
			{
				switch (player.rings[ntimes]->o_which)
				{
				when R_SEARCH:
					search();
				when R_TELEPORT:
					if (rnd(50) == 17)
						teleport();
					break;
				}
			}
		}
	}
}

//@ No need to declare in rogue.h
byte
com_char()
{
	bool same;
	byte ch;
	rogue::Turn &turn = game().turn;

	same = (turn.fast_mode == turn.fast_state);
	ch = readchar();
	if (same)
		turn.fast_mode = turn.fast_state;
	else
		turn.fast_mode = !turn.fast_state;
	switch (ch) {
		when '\b': ch = 'h';
		when '+': ch = 't';
		when '-': ch = 'z';
		break;
	}
	if (game().message.end && !turn.running)
		msg("");
	return ch;
}

//@ No need to declare in rogue.h
/*
 * Read a command, setting thing up according to prefix like devices
 * Return the command character to be executed.
 */
byte
get_prefix()
{
	int junk;
	byte retch, ch;
	rogue::Turn &turn = game().turn;

	turn.after = TRUE;
	turn.fast_mode = turn.fast_state;
	look(TRUE); //@ draw player in updated position on every non-sleep frame
	if (!turn.running)
		turn.door_stop = FALSE;
	turn.do_take = TRUE;
	turn.again = FALSE;
	if (--turn.count > 0) {
		turn.do_take = turn.last_take;
		retch = turn.last_ch;
		turn.fast_mode = FALSE;
		display().flush();  //@ repeated commands, ie, "10s"
	} else {
		turn.count = 0;
		if (turn.running) {
			retch = turn.run_dir;
			turn.do_take = turn.last_take;
			display().flush();  //@ running ("H", "fh", "L", etc)
		} else {
			for (retch = 0; retch == 0; ) {
				switch (ch = com_char()) {
					case '0': case '1': case '2': case '3': case '4':
					case '5': case '6': case '7': case '8': case '9':
						junk = turn.count * 10;
						if ((junk += ch - '0') > 0 && junk < 10000)
							turn.count = junk;
						show_count();
					when 'f':
						turn.fast_mode = !turn.fast_mode;
					when 'g':
						turn.do_take = FALSE;
					when 'a':
						retch = turn.last_ch;
						turn.count = turn.last_count;
						turn.do_take = turn.last_take;
						turn.again = TRUE;
					when ' ':	/* Spaces are ignored */
					when ESCAPE:
						turn.door_stop = FALSE;
						turn.count = 0;
						show_count();
					otherwise:
						retch = ch;
				}
			}
		}
	}
	if (turn.count)
		turn.fast_mode = FALSE;
	switch (retch) {
	case 'h': case 'j': case 'k': case 'l':
	case 'y': case 'u': case 'b': case 'n':
		if (turn.fast_mode && !turn.running ) {
			if (!on(game().player.body, ISBLIND)) {
				turn.door_stop = TRUE;
				turn.first_move = TRUE;
			}
			retch = toupper(retch);
		}
		/* fallthrough */
	case 'H': case 'J': case 'K': case 'L':
	case 'Y': case 'U': case 'B': case 'N':
	case 'q': case 'r': case 's': case 'z':
	case 't': case '.':
#ifdef WIZARD
	case CTRL(D): case 'C':
#endif //WIZARD
		break;
	default:
		turn.count = 0;
		break;
	}
	if (turn.count || turn.last_count)
		show_count();
	turn.last_ch = retch;
	turn.last_count = turn.count;
	turn.last_take = turn.do_take;
	return retch;
}

void
show_count()
{
	display().draw_count(game().turn.count);
}

void
execcom()
{
	coord mv;
	int ch;
	rogue::Turn &turn = game().turn;

	do {
		switch (ch = get_prefix()) {
		when 'h': case 'j': case 'k': case 'l':
		case 'y': case 'u': case 'b': case 'n':
			find_dir(ch, &mv);
			do_move(mv.y, mv.x);
		when 'H': case 'J': case 'K': case 'L':
		case 'Y': case 'U': case 'B': case 'N':
			do_run(tolower(ch));
		when 't':
			if (get_dir())
				missile(turn.delta.y, turn.delta.x);
			else
				turn.after = FALSE;
		when 'Q': turn.after = FALSE; quit();
		when 'i': turn.after = FALSE; inventory(pack, 0, "");
		when 'd': drop();
		when 'q': quaff();
		when 'r': read_scroll();
		when 'e': eat();
		when 'w': wield();
		when 'W': wear();
		when 'T': take_off();
		when 'P': ring_on();
		when 'R': ring_off();
		when 'c': turn.after = FALSE; call();
		when '>': turn.after = FALSE; d_level();
		when '<': turn.after = FALSE; u_level();
		when '/': turn.after = FALSE; help(helpobjs);
		when '?': turn.after = FALSE; help(helpcoms);
		when 's': search();
		when 'z':
			if (get_dir())
				do_zap();
			else
				turn.after = FALSE;
		when 'D': turn.after = FALSE; discovered();
		when CTRL('T'):
			turn.after = FALSE;
			msg((game().options.expert ^= 1)
				? "Ok, I'll be brief"
				: "Goodie, I can use big words again!");
		when 'F': turn.after = FALSE; do_macro(game().options.macro, MACROSZ);
		when CTRL('F'): turn.after = FALSE; turn.typeahead = game().options.macro;
		when CTRL('R'): turn.after = FALSE; msg(game().message.last);
		when 'v':
			turn.after = FALSE;
			msg("Rogue version %d.%d (Mr. Mctesq was here), dungeon %u", REV, VER,
				rogue::rng().seed());
		when 'S': turn.after = FALSE; save_game();
		when '.': doctor();
		when '^':
			turn.after = FALSE;
			if (get_dir()) {
				coord lookat;

				lookat.y = hero.y + turn.delta.y;
				lookat.x = hero.x + turn.delta.x;
				if (chat(lookat.y, lookat.x) != TRAP)
					msg("no trap there.");
				else
					msg("you found %s",
						tr_name(flat(lookat.y, lookat.x) & F_TMASK));
			}
		when 'o': turn.after = FALSE; msg("i don't have any options, oh my!");
		when CTRL('L'):
			turn.after = FALSE;
			msg("the screen looks fine to me (jll was here)");
#ifdef WIZARD
		when 'C': turn.after = FALSE; create_obj();
#endif
		otherwise:
			turn.after = FALSE;
			game().message.remember = FALSE;
			msg("illegal command '%s'", io_unctrl(ch));
			turn.count = 0;
			game().message.remember = TRUE;
		}
		if (turn.take && turn.do_take)
			pick_up(turn.take);
		turn.take = 0;
		if (!turn.running)
			turn.door_stop = FALSE;
	} while (turn.after == FALSE);
}
