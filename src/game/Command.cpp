#include "game/Command.hpp"

#include "glyphs.h"

namespace rogue {

namespace {

// The key table
struct Binding {
	int key;
	Command command;
};

constexpr Binding bindings[] = {
	{'h', Command::Move}, {'j', Command::Move}, {'k', Command::Move}, {'l', Command::Move},
	{'y', Command::Move}, {'u', Command::Move}, {'b', Command::Move}, {'n', Command::Move},
	{'H', Command::Run}, {'J', Command::Run}, {'K', Command::Run}, {'L', Command::Run},
	{'Y', Command::Run}, {'U', Command::Run}, {'B', Command::Run}, {'N', Command::Run},
	{'t', Command::Throw},
	{'Q', Command::Quit},
	{'i', Command::Inventory},
	{'d', Command::Drop},
	{'q', Command::Quaff},
	{'r', Command::Read},
	{'e', Command::Eat},
	{'w', Command::Wield},
	{'W', Command::Wear},
	{'T', Command::TakeOff},
	{'P', Command::PutOnRing},
	{'R', Command::RemoveRing},
	{'c', Command::Call},
	{'>', Command::Descend},
	{'<', Command::Ascend},
	{'/', Command::HelpObjects},
	{'?', Command::HelpCommands},
	{'s', Command::Search},
	{'z', Command::Zap},
	{'D', Command::Discoveries},
	{CTRL('T'), Command::ToggleBrief},
	{'F', Command::Macro},
	{CTRL('F'), Command::TypeMacro},
	{CTRL('R'), Command::RepeatMessage},
	{'v', Command::Version},
	{'S', Command::Save},
	{'.', Command::Rest},
	{'^', Command::IdentifyTrap},
	{'o', Command::Options},
	{CTRL('L'), Command::Redraw},
};

}  // namespace

Command
command_of(int key)
{
	for (const Binding &b : bindings)
		if (b.key == key)
			return b.command;
	return Command::Illegal;
}

bool
takes_turn(Command command)
{
	switch (command)
	{
	case Command::Move: case Command::Run:
	case Command::Throw: case Command::Zap:
	case Command::Drop: case Command::Quaff: case Command::Read:
	case Command::Eat: case Command::Wield: case Command::Wear:
	case Command::TakeOff: case Command::PutOnRing: case Command::RemoveRing:
	case Command::Search: case Command::Rest:
		return true;
	default:
		return false;
	}
}

/*
 * The original listed the keys (after get_prefix() turned a fast-mode step
 * into a run).
 */
bool
repeatable(Command command)
{
	switch (command)
	{
	case Command::Move: case Command::Run:
	case Command::Quaff: case Command::Read: case Command::Search:
	case Command::Zap: case Command::Throw: case Command::Rest:
		return true;
	default:
		return false;
	}
}

}  // namespace rogue
