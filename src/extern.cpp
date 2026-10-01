/*
 * global variable initializaton
 *
 * @(#)extern.c	5.2 (Berkeley) 6/16/82
 */

#include "rogue.h"

/*
 * Names of the various experience levels
 */

constexpr std::array<std::string_view, 21> he_man = std::to_array<std::string_view>({
	"",
	"Guild Novice",
	"Apprentice",
	"Journeyman",
	"Adventurer",
	"Fighter",
	"Warrior",
	"Rogue",
	"Champion",
	"Master Rogue",
	"Warlord",
	"Hero",
	"Guild Master",
	"Dragonlord",
	"Wizard",
	"Rogue Geek",
	"Rogue Addict",
	"Schmendrick",
	"Gunfighter",
	"Time Waster",
	"Bug Chaser"
});

/* bool askme = true; */			/* Ask about unidentified things */
/* bool fight_flush = true;	*/	/* True if toilet input */
/* bool jump = false;	*/		/* Show running as series of jumps */
/* bool passgo = true;	*/		/* Follow passages */
/* bool slow_invent = false; */		/* Inventory one line at a time */
/* char *release;	*/			/* Release number of rogue */
/* WINDOW *hw;				 Used as a scratch window */
