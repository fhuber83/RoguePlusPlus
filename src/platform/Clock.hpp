#pragma once

#include <chrono>

/*
 * The time of day, for the clock on the status line and the year on the
 * tombstone. (Was md_time() and md_localtime(), and their struct TM.)
 */

namespace rogue::platform {

/*
 * now:
 *	The time now, to the second.
 */
std::chrono::sys_seconds now();

/*
 * local_time:
 *	A time as the local clock shows it (the TZ variable, or the system's
 *	time zone).
 */
std::chrono::local_seconds local_time(std::chrono::sys_seconds time);

}  // namespace rogue::platform
