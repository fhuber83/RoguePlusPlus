#pragma once

#include <array>
#include <string_view>

/*
 * Experience levels: the experience each one needs, and the rank that comes
 * with it.
 */

namespace rogue::rules {

/*
 * The experience needed for each level: 10, doubling 18 times, then 0
 * to end the table
 */
inline constexpr std::array<long, 20> e_levels = {
	10L, 20L, 40L, 80L, 160L, 320L, 640L, 1280L, 2560L, 5120L, 10240L,
	20480L, 40960L, 81920L, 163840L, 327680L, 655360L, 1310720L, 2621440L, 0L,
};

/*
 * Names of the various experience levels: the rank, by experience level
 * (he_man[level - 1])
 */
inline constexpr std::array<std::string_view, 21> he_man = {
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
};

}  // namespace rogue::rules
