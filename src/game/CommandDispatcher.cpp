/*
 * Read and execute the user comamnds
 *
 * command.c	1.44	(A.I. Design)	2/14/85
 */

#include	"rogue.h"

namespace rogue {

//@ Set by resume_saved_game() until the first command after a restore
static bool resuming = false;

void
resume_saved_game()
{
	resuming = true;
}

void
command()
{
	int ntimes;
	rogue::Player &player = game().player;

	if (resuming)
		ntimes = 1;	//@ the save was made after this roll
	else if (player.body.t_flags.test(ISHASTE))
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
				case R_SEARCH:
					search();
					break;
				case R_TELEPORT:
					if (rnd(50) == 17)
						teleport();
					break;
				}
			}
		}
	}
}

//@ No need to declare in rogue.h
static unsigned char
com_char()
{
	bool same;
	unsigned char ch;
	rogue::Turn &turn = game().turn;

	same = (turn.fast_mode == turn.fast_state);
	ch = readchar();
	if (same)
		turn.fast_mode = turn.fast_state;
	else
		turn.fast_mode = !turn.fast_state;
	switch (ch) {
		case '\b': ch = 'h'; break;
		case '+': ch = 't'; break;
		case '-': ch = 'z';
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
static unsigned char
get_prefix()
{
	int junk;
	unsigned char retch, ch;
	rogue::Turn &turn = game().turn;

	turn.after = TRUE;
	turn.fast_mode = turn.fast_state;
	if (resuming)
		resuming = false;	//@ the save was made after this look()
	else
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
						break;
					case 'f':
						turn.fast_mode = !turn.fast_mode;
						break;
					case 'g':
						turn.do_take = FALSE;
						break;
					case 'a':
						retch = turn.last_ch;
						turn.count = turn.last_count;
						turn.do_take = turn.last_take;
						turn.again = TRUE;
						break;
					case ' ':	/* Spaces are ignored */ break;
					case ESCAPE:
						turn.door_stop = FALSE;
						turn.count = 0;
						show_count();
						break;
					default:
						retch = ch;
				}
			}
		}
	}
	if (turn.count)
		turn.fast_mode = FALSE;
	//@ Which commands a count repeats is in game/Command.cpp
	if (command_of(retch) == Command::Move && turn.fast_mode && !turn.running) {
		if (!game().player.body.t_flags.test(ISBLIND)) {
			turn.door_stop = TRUE;
			turn.first_move = TRUE;
		}
		retch = toupper(retch);
	}
	if (!repeatable(command_of(retch)))
		turn.count = 0;
	if (turn.count || turn.last_count)
		show_count();
	//@ Saving isn't repeated: a restored game repeats the command before it
	if (command_of(retch) != Command::Save) {
		turn.last_ch = retch;
		turn.last_count = turn.count;
		turn.last_take = turn.do_take;
	}
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
		ch = get_prefix();
		Command cmd = command_of(ch);
		//@ was a "turn.after = FALSE;" in each case that doesn't take a turn
		if (!takes_turn(cmd))
			turn.after = FALSE;
		switch (cmd) {
		case Command::Move:
			find_dir(ch, &mv);
			do_move(mv.y, mv.x);
			break;
		case Command::Run:
			do_run(tolower(ch));
			break;
		case Command::Throw:
			if (get_dir())
				missile(turn.delta.y, turn.delta.x);
			else
				turn.after = FALSE;
			break;
		case Command::Quit: quit(); break;
		case Command::Inventory: inventory(pack, ItemFilter::all(), ""); break;
		case Command::Drop: drop(); break;
		case Command::Quaff: quaff(); break;
		case Command::Read: read_scroll(); break;
		case Command::Eat: eat(); break;
		case Command::Wield: wield(); break;
		case Command::Wear: wear(); break;
		case Command::TakeOff: take_off(); break;
		case Command::PutOnRing: ring_on(); break;
		case Command::RemoveRing: ring_off(); break;
		case Command::Call: call(); break;
		case Command::Descend: d_level(); break;
		case Command::Ascend: u_level(); break;
		case Command::HelpObjects: help(helpobjs); break;
		case Command::HelpCommands: help(helpcoms); break;
		case Command::Search: search(); break;
		case Command::Zap:
			if (get_dir())
				do_zap();
			else
				turn.after = FALSE;
			break;
		case Command::Discoveries: discovered(); break;
		case Command::ToggleBrief:
			msg((game().options.expert ^= 1)
				? "Ok, I'll be brief"
				: "Goodie, I can use big words again!");
			break;
		case Command::Macro: do_macro(game().options.macro, MACROSZ); break;
		case Command::TypeMacro: turn.typeahead = game().options.macro; break;
		case Command::RepeatMessage: msg(game().message.last); break;
		case Command::Version:
			msg("Rogue version %d.%d (Mr. Mctesq was here), dungeon %u", REV, VER,
				rogue::rng().seed());
			break;
		case Command::Save: save_game(); break;
		case Command::Rest: doctor(); break;
		case Command::IdentifyTrap:
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
			break;
		case Command::Options: msg("i don't have any options, oh my!"); break;
		case Command::Redraw: msg("the screen looks fine to me (jll was here)"); break;
#ifdef WIZARD
		case Command::CreateObject: create_obj(); break;
#endif
		case Command::Illegal:
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

}  // namespace rogue
