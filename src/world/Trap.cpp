/*
 * The names of the traps.
 *
 * tr_name() comes from misc.c.
 *
 * misc.c	1.4		(A.I. Design)	12/14/84
 */

#include "rogue.h"

namespace rogue {

std::string_view
tr_name(Trap type)
{
	switch (type) {
	case Trap::Door:
		return "a trapdoor";
	case Trap::Bear:
		return "a beartrap";
	case Trap::Sleep:
		return "a sleeping gas trap";
	case Trap::Arrow:
		return "an arrow trap";
	case Trap::Teleport:
		return "a teleport trap";
	case Trap::Dart:
		return "a poison dart trap";
	}
	msg("wierd trap: {:d}", std::to_underlying(type));
	return "";
}

}  // namespace rogue
