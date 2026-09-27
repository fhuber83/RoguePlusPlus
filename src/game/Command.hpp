#pragma once

/*
 * The commands the player can give, and the keys that give them.
 *
 * The key is still what the command reads for a direction (h j k l y u b n)
 * and what "illegal command" prints, so the dispatcher gets both. Keys that
 * are aliases of another key (backspace for h, + for t, - for z) are turned
 * into that key by com_char(), and special keys (arrows, F1-F9, ...) by
 * readchar(), before they get here.
 *
 * Needs no legacy header, so tests can include it directly.
 */

namespace rogue {

enum class Command : unsigned char {
	Illegal,		/* no command has this key */
	Move,			/* h j k l y u b n: one step */
	Run,			/* H J K L Y U B N: run until something is seen */
	Throw,			/* t */
	Quit,			/* Q */
	Inventory,		/* i */
	Drop,			/* d */
	Quaff,			/* q */
	Read,			/* r */
	Eat,			/* e */
	Wield,			/* w */
	Wear,			/* W */
	TakeOff,		/* T */
	PutOnRing,		/* P */
	RemoveRing,		/* R */
	Call,			/* c: name an item kind */
	Descend,		/* > */
	Ascend,			/* < */
	HelpObjects,	/* /: what a glyph is */
	HelpCommands,	/* ? */
	Search,			/* s */
	Zap,			/* z */
	Discoveries,	/* D */
	ToggleBrief,	/* ^T */
	Macro,			/* F: type the macro now */
	TypeMacro,		/* ^F: queue the macro as typeahead */
	RepeatMessage,	/* ^R */
	Version,		/* v */
	Save,			/* S */
	Rest,			/* . */
	IdentifyTrap,	/* ^ */
	Options,		/* o */
	Redraw,			/* ^L */
};

// The command a key gives: Command::Illegal when it gives none.
Command command_of(int key);

/*
 * Whether the command uses up a turn. The ones that don't still let the
 * player give another command right away. Throw and Zap use one only when a
 * direction was given; the dispatcher handles that.
 */
bool takes_turn(Command command);

// Whether a count prefix ("10s") repeats the command; for any other command
// the count is dropped.
bool repeatable(Command command);

}  // namespace rogue
