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
	else if (on(player.body, ISHASTE))
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
static byte
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
static byte
get_prefix()
{
	int junk;
	byte retch, ch;
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
	//@ Which commands a count repeats is in game/Command.cpp
	if (command_of(retch) == Command::Move && turn.fast_mode && !turn.running) {
		if (!on(game().player.body, ISBLIND)) {
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
		when Command::Run:
			do_run(tolower(ch));
		when Command::Throw:
			if (get_dir())
				missile(turn.delta.y, turn.delta.x);
			else
				turn.after = FALSE;
		when Command::Quit: quit();
		when Command::Inventory: inventory(pack, ItemFilter::all(), "");
		when Command::Drop: drop();
		when Command::Quaff: quaff();
		when Command::Read: read_scroll();
		when Command::Eat: eat();
		when Command::Wield: wield();
		when Command::Wear: wear();
		when Command::TakeOff: take_off();
		when Command::PutOnRing: ring_on();
		when Command::RemoveRing: ring_off();
		when Command::Call: call();
		when Command::Descend: d_level();
		when Command::Ascend: u_level();
		when Command::HelpObjects: help(helpobjs);
		when Command::HelpCommands: help(helpcoms);
		when Command::Search: search();
		when Command::Zap:
			if (get_dir())
				do_zap();
			else
				turn.after = FALSE;
		when Command::Discoveries: discovered();
		when Command::ToggleBrief:
			msg((game().options.expert ^= 1)
				? "Ok, I'll be brief"
				: "Goodie, I can use big words again!");
		when Command::Macro: do_macro(game().options.macro, MACROSZ);
		when Command::TypeMacro: turn.typeahead = game().options.macro;
		when Command::RepeatMessage: msg(game().message.last);
		when Command::Version:
			msg("Rogue version %d.%d (Mr. Mctesq was here), dungeon %u", REV, VER,
				rogue::rng().seed());
		when Command::Save: save_game();
		when Command::Rest: doctor();
		when Command::IdentifyTrap:
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
		when Command::Options: msg("i don't have any options, oh my!");
		when Command::Redraw: msg("the screen looks fine to me (jll was here)");
#ifdef WIZARD
		when Command::CreateObject: create_obj();
#endif
		when Command::Illegal:
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
