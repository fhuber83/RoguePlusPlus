/*
 * Contains functions for dealing with things that happen in the
 * future.
 *
 * (#)daemon.c	5.2 (Berkeley) 6/18/82
 */

#include "rogue.h"

#define EMPTY	0
#define FULL	1
#define DAEMON -1
#define MAXDAEMONS rogue::Scheduler::max_actions

/*@
 * struct delayed_action, as well as functions using it as return type such
 * as d_slot() and find_slot() are now marked static as the struct is not
 * declared in rogue.h, and they are only used in this file
 */

/*@
 * `int d_arg` member was removed as all fuses and daemons have no arguments,
 * and for the only one that did, turn_see(), the argument type is bool. It's
 * also now wrapped and no longer directly used as fuse, as its return type is
 * not void, making the argument member of this struct unneeded.
 *
 * A solution to handle generic functions of multiple return and argument types
 * would be a somewhat complex approach using unions to simulate overload, a
 * sophistication not needed for Rogue.
 */
//@ The slots (were the static d_list) live in game().scheduler
using delayed_action = rogue::Scheduler::Action;

static delayed_action *d_begin() { return game().scheduler.actions; }
static delayed_action *d_end() { return d_begin() + MAXDAEMONS; }

/*
 * d_slot:
 *	Find an empty slot in the daemon/fuse list
 */
static
delayed_action *
d_slot(void)
{
	delayed_action *dev;

	for (dev = d_begin(); dev < d_end(); dev++)
		if (dev->func == EMPTY)
			return dev;
#ifdef DEBUG
	debug("Ran out of fuse slots");
#endif
	return NULL;
}

/*
 * find_slot:
 *	Find a particular slot in the table
 */
static
delayed_action *
find_slot(void (*func)())
{
	delayed_action *dev;

	for (dev = d_begin(); dev < d_end(); dev++)
	if (func == dev->func)
		return dev;
	return NULL;
}

/*
 * daemon:
 *	Start a daemon, takes a function.
 */
void
start_daemon(void (*func)())
{
	delayed_action *dev;

	dev = d_slot();
	dev->func = func;
	dev->time = DAEMON;
}

/*
 * do_daemons:
 *	Run all the daemons, passing the argument to the function.
 */
void
do_daemons(void)
{
	delayed_action *dev;

	/*
	 * Loop through the devil list
	 */
	for (dev = d_begin(); dev < d_end(); dev++)
	{
		/*
		 * Executing each one, giving it the proper arguments
		 * @ Sorry, no more "arguments". And it was a single one.
		 */
		if (dev->time == DAEMON && dev->func != EMPTY)
		{
			(*dev->func)();
		}
	}
}

/*
 * fuse:
 *	Start a fuse to go off in a certain number of turns
 */
void
fuse(void (*func)(), int time)
{
	delayed_action *wire;

	wire = d_slot();
	wire->func = func;
	wire->time = time;
}

/*
 * lengthen:
 *	Increase the time until a fuse goes off
 */
void
lengthen(void (*func)(), int xtime)
{
	delayed_action *wire;

	if ((wire = find_slot(func)) == NULL)
		return;
	wire->time += xtime;
}

/*
 * extinguish:
 *	Put out a fuse
 */
void
extinguish(void (*func)())
{
	delayed_action *wire;

	if ((wire = find_slot(func)) == NULL)
		return;
	wire->func = EMPTY;
}

/*
 * do_fuses:
 *	Decrement counters and start needed fuses
 */
void
do_fuses(void)
{
	delayed_action *wire;

	/*
	 * Step though the list
	 */
	for (wire = d_begin(); wire < d_end(); wire++) {
	/*
	 * Decrementing counters and starting things we want.  We also need
	 * to remove the fuse from the list once it has gone off.
	 */
		if (wire->func != EMPTY && wire->time > 0 && --wire->time == 0)
		{
			(*wire->func)();
			wire->func = EMPTY;
		}
	}
}
