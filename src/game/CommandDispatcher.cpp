/*
 * Read and execute the user comamnds
 *
 * command.c	1.44	(A.I. Design)	2/14/85
 */

#include "game/CommandDispatcher.hpp"

#include <optional>

#include "core/Ascii.hpp"
#include "core/Coord.hpp"
#include "core/Glyphs.hpp"
#include "core/KindTable.hpp"
#include "core/Text.hpp"
#include "entities/Creature.hpp"
#include "entities/Item.hpp"
#include "game/Command.hpp"
#include "game/Game.hpp"
#include "game/GameLoop.hpp"
#include "game/Help.hpp"
#include "game/Keyboard.hpp"
#include "game/Messages.hpp"
#include "game/Movement.hpp"
#include "game/PlayerCommands.hpp"
#include "game/StatusLine.hpp"
#include "items/Identification.hpp"
#include "items/Inventory.hpp"
#include "items/Kinds.hpp"
#include "items/effects/Armor.hpp"
#include "items/effects/Potion.hpp"
#include "items/effects/Ring.hpp"
#include "items/effects/Scroll.hpp"
#include "items/effects/Wand.hpp"
#include "items/effects/Weapon.hpp"
#include "persistence/SaveCommands.hpp"
#include "rules/Hunger.hpp"
#include "rules/Regeneration.hpp"
#include "rules/Scheduler.hpp"
#include "ui/Display.hpp"
#include "world/Level.hpp"
#include "world/Look.hpp"
#include "world/Rooms.hpp"
#include "world/Traps.hpp"

namespace rogue {

namespace {

constexpr int REV = 1;		/* the version, 1.48 */
constexpr int VER = 48;

}  // namespace

void
resume_saved_game()
{
	game().turn.resuming = true;
}

void
command()
{
	rogue::Player &player = game().player;
	rogue::Turn &turn = game().turn;

	/*
	 * A hasted rogue gets two or three moves a command, rolled as it
	 * starts. A restored game goes on with the command the save was made
	 * in, and the moves it had left.
	 */
	if (!turn.resuming || turn.moves_left == 0)
		turn.moves_left = player.body.is(CreatureFlag::Hasted) ? rnd(2) + 2 : 1;
	for (; turn.moves_left > 0; turn.moves_left--) {
		status();
		if (player.no_command) {
			if (--player.no_command <= 0) {
				msg("you can move again");
				player.no_command = 0;
			}
			ui::display().flush();  // sleeping, fainted, frozen, etc
		} else
			execcom();
		rules::do_fuses();
		rules::do_daemons();
		for (Hand hand : kinds<Hand>())
		{
			if (player.ring_item(hand))
			{
				switch (player.ring_item(hand)->which<Ring>())
				{
				case Ring::Searching:
					world::search();
					break;
				case Ring::Teleportation:
					if (rnd(50) == 17)
						world::teleport();
					break;
				default:
					break;
				}
			}
		}
	}
}

namespace {

unsigned char
com_char()
{
	rogue::Turn &turn = game().turn;

	bool same = (turn.fast_mode == turn.fast_state);
	unsigned char ch = readchar();
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

/*
 * Read a command, setting thing up according to prefix like devices
 * Return the command character to be executed.
 */
unsigned char
get_prefix()
{
	unsigned char retch;
	rogue::Turn &turn = game().turn;

	turn.after = true;
	turn.fast_mode = turn.fast_state;
	if (turn.resuming)
		turn.resuming = false;	// the save was made after this look()
	else
		world::look(true); // draw player in updated position on every non-sleep frame
	if (!turn.running)
		turn.door_stop = false;
	turn.do_take = true;
	turn.again = false;
	if (--turn.count > 0) {
		turn.do_take = turn.last_take;
		retch = turn.last_ch;
		turn.fast_mode = false;
		ui::display().flush();  // repeated commands, ie, "10s"
	} else {
		turn.count = 0;
		if (turn.running) {
			retch = turn.run_dir;
			turn.do_take = turn.last_take;
			ui::display().flush();  // running ("H", "fh", "L", etc)
		} else {
			for (retch = 0; retch == 0; ) {
				switch (unsigned char ch = com_char()) {
					case '0': case '1': case '2': case '3': case '4':
					case '5': case '6': case '7': case '8': case '9': {
						int junk = turn.count * 10;
						if ((junk += ch - '0') > 0 && junk < 10000)
							turn.count = junk;
						show_count();
						break;
					}
					case 'f':
						turn.fast_mode = !turn.fast_mode;
						break;
					case 'g':
						turn.do_take = false;
						break;
					case 'a':
						retch = turn.last_ch;
						turn.count = turn.last_count;
						turn.do_take = turn.last_take;
						turn.again = true;
						break;
					case ' ':	/* Spaces are ignored */ break;
					case ESCAPE:
						turn.door_stop = false;
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
		turn.fast_mode = false;
	// Which commands a count repeats is in game/Command.cpp
	if (command_of(retch) == Command::Move && turn.fast_mode && !turn.running) {
		if (!game().player.body.is(CreatureFlag::Blind)) {
			turn.door_stop = true;
			turn.first_move = true;
		}
		retch = to_upper(retch);
	}
	if (!repeatable(command_of(retch)))
		turn.count = 0;
	if (turn.count || turn.last_count)
		show_count();
	// Saving isn't repeated: a restored game repeats the command before it
	if (command_of(retch) != Command::Save) {
		turn.last_ch = retch;
		turn.last_count = turn.count;
		turn.last_take = turn.do_take;
	}
	return retch;
}

}  // namespace

void
show_count()
{
	ui::display().draw_count(game().turn.count);
}

void
execcom()
{
	rogue::Turn &turn = game().turn;
	rogue::Player &player = game().player;
	world::Level &level = game().level;

	do {
		int ch = get_prefix();
		Command cmd = command_of(ch);
		if (!takes_turn(cmd))
			turn.after = false;
		switch (cmd) {
		case Command::Move:
			if (std::optional<Coord> mv = find_dir(ch))	// a move key is a direction
				do_move(mv->y, mv->x);
			break;
		case Command::Run:
			do_run(to_lower(ch));
			break;
		case Command::Throw:
			if (get_dir())
				items::effects::missile(turn.delta.y, turn.delta.x);
			else
				turn.after = false;
			break;
		case Command::Quit: quit(); break;
		case Command::Inventory: items::inventory(player.body.pack, ItemFilter::all(), ""); break;
		case Command::Drop: items::drop(); break;
		case Command::Quaff: items::effects::quaff(); break;
		case Command::Read: items::effects::read_scroll(); break;
		case Command::Eat: rules::eat(); break;
		case Command::Wield: items::effects::wield(); break;
		case Command::Wear: items::effects::wear(); break;
		case Command::TakeOff: items::effects::take_off(); break;
		case Command::PutOnRing: items::effects::ring_on(); break;
		case Command::RemoveRing: items::effects::ring_off(); break;
		case Command::Call: items::call(); break;
		case Command::Descend: d_level(); break;
		case Command::Ascend: u_level(); break;
		case Command::HelpObjects: help(helpobjs); break;
		case Command::HelpCommands: help(helpcoms); break;
		case Command::Search: world::search(); break;
		case Command::Zap:
			if (get_dir())
				items::effects::do_zap();
			else
				turn.after = false;
			break;
		case Command::Discoveries: items::discovered(); break;
		case Command::ToggleBrief:
			msg("{}", (game().options.expert ^= 1)
				? "Ok, I'll be brief"
				: "Goodie, I can use big words again!");
			break;
		case Command::Macro: do_macro(game().options.macro); break;
		case Command::TypeMacro: turn.typeahead = game().options.macro; break;
		case Command::RepeatMessage: msg("{}", game().message.last); break;
		case Command::Version:
			msg("Rogue version {}.{} (Mr. Mctesq was here), dungeon {}", REV, VER,
				rogue::rng().seed());
			break;
		case Command::Save: persistence::save_game(); break;
		case Command::Rest: rules::doctor(); break;
		case Command::IdentifyTrap:
			if (get_dir()) {
				Coord lookat = player.body.pos + turn.delta;
				if (level.at(lookat) != TRAP)
					msg("no trap there.");
				else
					msg("you found {}",
						world::tr_name(level.flags_at(lookat).trap()));
			}
			break;
		case Command::Options: msg("i don't have any options, oh my!"); break;
		case Command::Redraw: msg("the screen looks fine to me (jll was here)"); break;
		case Command::Illegal:
			game().message.remember = false;
			msg("illegal command '{}'", io_unctrl(ch));
			turn.count = 0;
			game().message.remember = true;
		}
		if (turn.take && turn.do_take)
			items::pick_up(turn.take);
		turn.take = 0;
		if (!turn.running)
			turn.door_stop = false;
	} while (turn.after == false);
}

}  // namespace rogue
