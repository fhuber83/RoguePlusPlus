#include <gtest/gtest.h>

#include <vector>

#include "rules/Scheduler.hpp"

using rogue::rules::Event;
using rogue::rules::Scheduler;

namespace {

// Runs one turn the way command() does (fuses, then daemons) and returns
// what fired, in order.
std::vector<Event> turn(Scheduler &s)
{
	std::vector<Event> fired;
	auto record = [&](Event e) { fired.push_back(e); };
	s.run_fuses(record);
	s.run_daemons(record);
	return fired;
}

}  // namespace

TEST(Scheduler, DaemonsRunEveryTurnInSlotOrder)
{
	Scheduler s;
	s.start_daemon(Event::Doctor);
	s.start_daemon(Event::Stomach);
	s.start_daemon(Event::Runners);
	for (int i = 0; i < 3; i++)
		EXPECT_EQ(turn(s), (std::vector{Event::Doctor, Event::Stomach, Event::Runners}));
	EXPECT_EQ(s.time_left(Event::Doctor), -1);
}

TEST(Scheduler, FuseGoesOffOnceAfterItsTime)
{
	Scheduler s;
	s.fuse(Event::NoHaste, 3);
	EXPECT_TRUE(turn(s).empty());
	EXPECT_TRUE(turn(s).empty());
	EXPECT_EQ(turn(s), std::vector{Event::NoHaste});
	EXPECT_FALSE(s.is_set(Event::NoHaste));
	EXPECT_TRUE(turn(s).empty());
}

TEST(Scheduler, LengthenAndExtinguish)
{
	Scheduler s;
	s.fuse(Event::Unconfuse, 2);
	s.lengthen(Event::Unconfuse, 5);
	EXPECT_EQ(s.time_left(Event::Unconfuse), 7);
	s.extinguish(Event::Unconfuse);
	EXPECT_FALSE(s.is_set(Event::Unconfuse));
	// Both do nothing for an event that is not scheduled.
	s.lengthen(Event::Sight, 5);
	s.extinguish(Event::Sight);
	EXPECT_FALSE(s.is_set(Event::Sight));
}

// A freed slot is reused before later ones, so the order events run in
// depends on when they were scheduled, as in the original table.
TEST(Scheduler, FreedSlotIsReusedFirst)
{
	Scheduler s;
	s.start_daemon(Event::Doctor);
	s.start_daemon(Event::RollWander);
	s.start_daemon(Event::Stomach);
	s.extinguish(Event::RollWander);
	s.start_daemon(Event::Runners);
	EXPECT_EQ(turn(s), (std::vector{Event::Doctor, Event::Runners, Event::Stomach}));
}

// swander's fuse starts the rollwand daemon while its own slot is still
// taken, so rollwand gets a later slot and runs as a daemon the same turn.
TEST(Scheduler, FuseSlotStaysTakenWhileItFires)
{
	Scheduler s;
	s.fuse(Event::Swander, 1);
	std::vector<Event> fired;
	s.run_fuses([&](Event e) {
		fired.push_back(e);
		if (e == Event::Swander)
			s.start_daemon(Event::RollWander);
	});
	s.run_daemons([&](Event e) { fired.push_back(e); });
	EXPECT_EQ(fired, (std::vector{Event::Swander, Event::RollWander}));
	EXPECT_FALSE(s.is_set(Event::Swander));
	EXPECT_EQ(s.time_left(Event::RollWander), -1);
}

TEST(Scheduler, FullTableDropsNewEvents)
{
	Scheduler s;
	for (int i = 0; i < Scheduler::max_actions; i++)
		s.fuse(Event::Sight, 10);
	s.start_daemon(Event::Doctor);
	EXPECT_FALSE(s.is_set(Event::Doctor));
}
