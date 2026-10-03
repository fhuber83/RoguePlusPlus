/*
 * The time of day.
 *
 * mach_dep.c	1.4 (A.I. Design) 12/1/84
 */

#include "platform/Clock.hpp"

#include <chrono>
#include <ctime>

namespace rogue::platform {

std::chrono::sys_seconds
now()
{
	return std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
}

std::chrono::local_seconds
local_time(std::chrono::sys_seconds time)
{
	/*
	 * localtime_r() rather than std::chrono::current_zone(): it follows
	 * TZ as the game always did, and needs no time zone database
	 */
	std::time_t secs = std::chrono::system_clock::to_time_t(time);
	std::tm local;
	localtime_r(&secs, &local);
	return std::chrono::local_seconds{time.time_since_epoch() + std::chrono::seconds(local.tm_gmtoff)};
}

}  // namespace rogue::platform
