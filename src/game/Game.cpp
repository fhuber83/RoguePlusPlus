#include <algorithm>
#include <functional>

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

namespace {

// Whether p points into the n elements at first (std::less orders any pointers)
template <class T>
bool points_into(const T *p, const T *first, std::size_t n)
{
	std::less<const T *> less;
	return !less(p, first) && less(p, first + n);
}

bool is_room(const Level &level, const struct room *rp)
{
	return points_into(rp, level.rooms, MAXROOMS) || points_into(rp, level.passages, MAXPASS);
}

} // namespace

std::vector<std::string>
pool_problems(const Game &g)
{
	std::vector<std::string> problems;
	auto problem = [&](std::string text) { problems.push_back(std::move(text)); };
	const Pool &pool = g.pool;
	const Level &level = g.level;
	int item_refs[MAXITEMS] = {};
	int creature_refs[MAXITEMS] = {};

	auto count_items = [&](const List<Item> &list, const char *where) {
		for (const Item *obj : list) {
			int slot = pool.items.slot_of(obj);
			if (slot < 0)
				problem(std::string("an item outside the pool in ") + where);
			else
				item_refs[slot]++;
		}
	};
	count_items(level.objects, "the level's objects");
	count_items(g.player.body.t_pack, "the rogue's pack");
	for (const Creature *tp : level.monsters) {
		int slot = pool.creatures.slot_of(tp);
		if (slot < 0) {
			problem("a monster outside the pool");
			continue;
		}
		creature_refs[slot]++;
		count_items(tp->t_pack, "a monster's pack");

		const coord *dest = tp->t_dest;
		bool dest_ok = dest == nullptr || dest == &g.player.body.t_pos;
		for (const struct room &rp : level.rooms)
			dest_ok = dest_ok || dest == &rp.r_gold;
		for (const struct room &rp : level.passages)
			dest_ok = dest_ok || dest == &rp.r_gold;
		for (const Item *obj : level.objects)
			dest_ok = dest_ok || dest == &obj->o_pos;
		if (!dest_ok)
			problem("monster " + std::to_string(slot) + " is after something that isn't the hero, gold or a floor item");
		if (tp->t_room != nullptr && !is_room(level, tp->t_room))
			problem("monster " + std::to_string(slot) + " is in a room that isn't one");
	}

	int used = 0;
	for (int i = 0; i < MAXITEMS; i++) {
		bool item_used = pool.items.used(i), creature_used = pool.creatures.used(i);
		used += item_used + creature_used;
		if (item_used ? item_refs[i] != 1 : item_refs[i] != 0)
			problem("item " + std::to_string(i) + (item_used ? " is in use" : " is free")
				+ " and listed " + std::to_string(item_refs[i]) + " times");
		if (creature_used ? creature_refs[i] != 1 : creature_refs[i] != 0)
			problem("creature " + std::to_string(i) + (creature_used ? " is in use" : " is free")
				+ " and listed " + std::to_string(creature_refs[i]) + " times");
	}
	if (used != pool.total)
		problem("the pool counts " + std::to_string(pool.total) + " things in use, but " + std::to_string(used) + " are");

	const Player &player = g.player;
	for (const Item *worn : {player.armor, player.weapon, player.rings[0], player.rings[1]})
		if (worn != nullptr && !player.body.t_pack.contains(worn))
			problem("a worn item isn't in the pack");
	if (g.turn.last_item != nullptr && pool.items.slot_of(g.turn.last_item) < 0)
		problem("the item picked last isn't in use");
	if (player.body.t_room != nullptr && !is_room(level, player.body.t_room))
		problem("the rogue is in a room that isn't one");
	if (player.old_room != nullptr && !is_room(level, player.old_room))
		problem("the rogue was in a room that isn't one");
	return problems;
}

Game &game()
{
	static Game instance;
	return instance;
}

}  // namespace rogue
