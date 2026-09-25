/*
 * Contains functions for dealing with things that happen in the
 * future.
 *
 * (#)daemon.c	5.2 (Berkeley) 6/18/82
 */

#include "rogue.h"

namespace rogue::rules {

/*
 * d_slot:
 *	Find an empty slot in the daemon/fuse list
 */
Scheduler::Action *
Scheduler::free_slot()
{
	return find(Event::None);
}

/*
 * find_slot:
 *	Find a particular slot in the table
 */
Scheduler::Action *
Scheduler::find(Event event)
{
	for (Action &dev : actions)
		if (dev.event == event)
			return &dev;
	return nullptr;
}

const Scheduler::Action *
Scheduler::find(Event event) const
{
	return const_cast<Scheduler *>(this)->find(event);
}

/*
 * daemon:
 *	Start a daemon, takes a function.
 */
void
Scheduler::start_daemon(Event event)
{
	fuse(event, daemon_time);
}

/*
 * fuse:
 *	Start a fuse to go off in a certain number of turns
 */
void
Scheduler::fuse(Event event, int time)
{
	Action *wire = free_slot();

	if (wire == nullptr)
	{
#ifdef DEBUG
		debug("Ran out of fuse slots");
#endif
		return;
	}
	wire->event = event;
	wire->time = time;
}

/*
 * lengthen:
 *	Increase the time until a fuse goes off
 */
void
Scheduler::lengthen(Event event, int xtime)
{
	if (Action *wire = find(event))
		wire->time += xtime;
}

/*
 * extinguish:
 *	Put out a fuse
 */
void
Scheduler::extinguish(Event event)
{
	if (Action *wire = find(event))
		wire->event = Event::None;
}

int
Scheduler::time_left(Event event) const
{
	const Action *wire = find(event);
	return wire == nullptr ? 0 : wire->time;
}

std::array<Scheduler::Slot, Scheduler::max_actions>
Scheduler::slots() const
{
	std::array<Slot, max_actions> out;
	for (int i = 0; i < max_actions; i++)
		out[i] = Slot{actions[i].event, actions[i].time};
	return out;
}

void
Scheduler::set_slots(const std::array<Slot, max_actions> &slots)
{
	for (int i = 0; i < max_actions; i++)
		actions[i] = Action{slots[i].event, slots[i].time};
}

void start_daemon(Event event) { game().scheduler.start_daemon(event); }
void fuse(Event event, int time) { game().scheduler.fuse(event, time); }
void lengthen(Event event, int xtime) { game().scheduler.lengthen(event, xtime); }
void extinguish(Event event) { game().scheduler.extinguish(event); }
void do_daemons() { game().scheduler.run_daemons(fire); }
void do_fuses() { game().scheduler.run_fuses(fire); }

void
fire(Event event)
{
	switch (event)
	{
		case Event::Doctor: doctor();
		when Event::Stomach: stomach();
		when Event::Runners: runners();
		when Event::Swander: swander();
		when Event::RollWander: rollwand();
		when Event::Unconfuse: unconfuse();
		when Event::Unsee: unsee();
		when Event::Sight: sight();
		when Event::NoHaste: nohaste();
		when Event::TurnSeeOff: turn_see(TRUE);
		when Event::None: break;
	}
}

}  // namespace rogue::rules
