#pragma once

/*
 * Daemons (run every turn) and fuses (go off once after a number of turns).
 *
 * The original daemon.c kept a table of function pointers. The slots now hold
 * an Event, and fire() (Scheduler.cpp) maps each event to the function it
 * runs. The table keeps its size and slot order, so events run in the same
 * order as before, and an event added while the table is being walked lands
 * in the first free slot, just as it did.
 *
 * Needs no legacy header, so tests can include it directly.
 */

namespace rogue::rules {

// Everything that can be scheduled. The comment names the function it runs.
enum class Event : unsigned char {
	None,			/* free slot */
	Doctor,			/* doctor(): regain hit points */
	Stomach,		/* stomach(): digest food */
	Runners,		/* runners(): move the chasing monsters */
	Swander,		/* swander(): start rolling for wandering monsters */
	RollWander,		/* rollwand(): maybe bring a wandering monster */
	Unconfuse,		/* unconfuse(): confusion wears off */
	Unsee,			/* unsee(): see invisible wears off */
	Sight,			/* sight(): blindness wears off */
	NoHaste,		/* nohaste(): haste self wears off */
	TurnSeeOff,		/* turn_see(TRUE): monster detection wears off */
};

class Scheduler {
public:
	static constexpr int max_actions = 20;	/* MAXDAEMONS */

	// Run event every turn. Does nothing when every slot is taken (the
	// original dereferenced a null slot; the game never uses more than 9).
	void start_daemon(Event event);

	// Run event once, after time turns.
	void fuse(Event event, int time);

	// Delay the first slot holding event by xtime turns, if there is one.
	void lengthen(Event event, int xtime);

	// Free the first slot holding event, if there is one.
	void extinguish(Event event);

	// Whether some slot holds event.
	bool is_set(Event event) const { return find(event) != nullptr; }

	// Turns until the first fuse holding event goes off; -1 for a daemon, 0
	// when the event is not scheduled.
	int time_left(Event event) const;

	/*
	 * do_daemons:
	 *	Call fire(event) for every daemon, in slot order.
	 */
	template <typename Fire>
	void run_daemons(Fire &&fire)
	{
		for (Action &dev : actions)
			if (dev.time == daemon_time && dev.event != Event::None)
				fire(dev.event);
	}

	/*
	 * do_fuses:
	 *	Count every fuse down, and fire and free the ones that reach zero.
	 *	The slot stays taken while its event runs.
	 */
	template <typename Fire>
	void run_fuses(Fire &&fire)
	{
		for (Action &wire : actions)
			if (wire.event != Event::None && wire.time > 0 && --wire.time == 0)
			{
				fire(wire.event);
				wire.event = Event::None;
			}
	}

private:
	static constexpr int daemon_time = -1;	/* DAEMON */

	struct Action {
		Event event = Event::None;	/* d_func */
		int time = 0;				/* d_time: turns left, or daemon_time */
	};

	Action *free_slot();
	Action *find(Event event);
	const Action *find(Event event) const;

	Action actions[max_actions] = {};	/* d_list */
};

/*
 * The scheduler of the game being played (game().scheduler). The legacy
 * names are brought into the global namespace by rogue.h.
 */
void start_daemon(Event event);
void fuse(Event event, int time);
void lengthen(Event event, int xtime);
void extinguish(Event event);
void do_daemons();
void do_fuses();

// Run what event stands for.
void fire(Event event);

}  // namespace rogue::rules
