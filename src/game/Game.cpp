#include <algorithm>

#include "rogue.h"

namespace rogue {

Items::Items()
{
	std::copy_n(s_magic_base, MAXSCROLLS, s_magic);
	std::copy_n(p_magic_base, MAXPOTIONS, p_magic);
	std::copy_n(r_magic_base, MAXRINGS, r_magic);
	std::copy_n(ws_magic_base, MAXSTICKS, ws_magic);
	std::copy_n(things_base, NUMTHINGS, things);
}

Game &game()
{
	static Game instance;
	return instance;
}

}  // namespace rogue
