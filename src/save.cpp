/*
 * save and restore routines
 *
 * save.c	1.32	(A.I. Design)	12/13/84
 */

/*
 *  routines for saving a program in any given state.
 *  This is the first pass of this, so I have no idea
 *  how it is really going to work.
 *  The two basic functions here will be "save" and "restor".
 */

/*@
 * The original saved and restored a raw dump of the game's memory, which
 * could not work on a modern system and was already disabled by the port.
 * The dead code was removed in phase 4 (see git history before that). Phase 8
 * of docs/MODERNIZATION.md replaces this with real serialization.
 */

#include	"rogue.h"

void
save_game()
{
	display().write("Sorry, saving games is disabled. Patches are welcome!");
}

void
restore(char *)
{
	fatal("Sorry, restoring games is disabled. Patches are welcome!\n");
}
