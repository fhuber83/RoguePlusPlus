#pragma once

#include <string_view>

#include "core/KindTable.hpp"

namespace rogue {

/*
 * The kinds of trap. A trap's number is kept in the low bits of its square's
 * MapFlags (MapFlags::trap()).
 */
enum class Trap {
	Door,		/* T_DOOR: a trapdoor to the next level */
	Arrow,		/* T_ARROW */
	Sleep,		/* T_SLEEP: sleeping gas */
	Bear,		/* T_BEAR */
	Teleport,	/* T_TELEP */
	Dart,		/* T_DART: a poison dart */
};
template <>
inline constexpr std::size_t kind_count<Trap> = 6;

/*
 * tr_name:
 *	The name of a trap, with its article ("a beartrap"). An unknown kind
 *	says so on the message line and has the name "".
 */
std::string_view tr_name(Trap type);

}  // namespace rogue
