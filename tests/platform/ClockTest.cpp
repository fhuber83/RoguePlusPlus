#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <optional>
#include <string>

#include <stdlib.h>
#include <time.h>

#include "platform/Clock.hpp"

using namespace std::chrono;

namespace {

// Sets TZ (a POSIX rule, which needs no time zone database) while it lives.
class ScopedTz {
public:
	explicit ScopedTz(const char *tz)
	{
		if (const char *old = std::getenv("TZ"))
			old_ = old;
		setenv("TZ", tz, 1);
		tzset();
	}
	~ScopedTz()
	{
		if (old_)
			setenv("TZ", old_->c_str(), 1);
		else
			unsetenv("TZ");
		tzset();
	}

private:
	std::optional<std::string> old_;
};

}  // namespace

TEST(Clock, NowIsWholeSeconds)
{
	sys_seconds t = rogue::platform::now();
	EXPECT_GT(t.time_since_epoch().count(), 0);
}

TEST(Clock, LocalTimeFollowsTz)
{
	sys_seconds t = sys_days{year{2026} / 10 / 1} + hours{22} + minutes{30} + seconds{15};
	{
		ScopedTz tz("UTC0");
		EXPECT_EQ(rogue::platform::local_time(t).time_since_epoch(), t.time_since_epoch());
	}
	{
		ScopedTz tz("XYZ-2");	// two hours east of Greenwich
		local_seconds local = rogue::platform::local_time(t);
		EXPECT_EQ(local.time_since_epoch() - t.time_since_epoch(), hours{2});
		// past midnight: the next day, as the tombstone's year would see it
		year_month_day date{floor<days>(local)};
		EXPECT_EQ(date, year{2026} / 10 / 2);
		hh_mm_ss hms{local - floor<days>(local)};
		EXPECT_EQ(hms.hours(), hours{0});
		EXPECT_EQ(hms.minutes(), minutes{30});
		EXPECT_EQ(hms.seconds(), seconds{15});
	}
}
