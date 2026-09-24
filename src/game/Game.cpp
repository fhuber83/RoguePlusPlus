#include "rogue.h"

namespace rogue {

Game &game()
{
	static Game instance;
	return instance;
}

}  // namespace rogue
