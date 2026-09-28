#include <algorithm>
#include <functional>

#include "rogue.h"

namespace rogue {

Items::Items()
{
	s_magic = s_magic_base;
	p_magic = p_magic_base;
	r_magic = r_magic_base;
	ws_magic = ws_magic_base;
	std::copy_n(things_base, NUMTHINGS, things);
}


std::vector<std::string>
pool_problems(const Game &g)
{
	std::vector<std::string> problems;
	auto problem = [&](std::string text) { problems.push_back(std::move(text)); };
	const Pool &pool = g.pool;
	const Level &level = g.level;
	int item_refs[MAXITEMS] = {};
	int creature_refs[MAXITEMS] = {};

	auto count_items = [&](const List<Item> &list, std::string_view where) {
		for (const Item *obj : list) {
			int slot = pool.items.slot_of(obj);
			if (slot < 0)
				problem(std::format("an item outside the pool in {}", where));
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

		const std::optional<Destination> &dest = tp->t_dest;
		bool dest_ok = !dest || std::holds_alternative<Hero>(*dest)
			|| (std::holds_alternative<Gold>(*dest) && Level::valid(std::get<Gold>(*dest).room));
		if (dest && std::holds_alternative<ItemId>(*dest))
			dest_ok = level.objects.contains(pool.item(std::optional<ItemId>(std::get<ItemId>(*dest))));
		if (!dest_ok)
			problem("monster " + std::to_string(slot) + " is after something that isn't the hero, gold or a floor item");
		if (tp->t_room && !Level::valid(*tp->t_room))
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
	for (std::optional<ItemId> worn : {player.armor, player.weapon, player.rings[Hand::Left], player.rings[Hand::Right]})
		if (worn && (pool.item(worn) == nullptr || !player.body.t_pack.contains(pool.item(worn))))
			problem("a worn item isn't in the pack");
	if (g.turn.last_item && pool.item(g.turn.last_item) == nullptr)
		problem("the item picked last isn't in use");
	if (player.body.t_room && !Level::valid(*player.body.t_room))
		problem("the rogue is in a room that isn't one");
	if (player.old_room && !Level::valid(*player.old_room))
		problem("the rogue was in a room that isn't one");
	return problems;
}

Coord Game::where(const Destination &dest) const
{
	if (std::holds_alternative<Hero>(dest))
		return player.body.t_pos;
	if (std::holds_alternative<Gold>(dest))
		return level.room(std::get<Gold>(dest).room).r_gold;
	return pool.item(std::get<ItemId>(dest)).o_pos;
}

Item *Player::armor_item() const { return game().pool.item(armor); }
Item *Player::weapon_item() const { return game().pool.item(weapon); }
Item *Player::ring_item(Hand hand) const { return game().pool.item(rings[hand]); }

bool Player::wears(Hand hand, Ring ring) const
{
	const Item *obj = ring_item(hand);
	return obj != nullptr && obj->which<Ring>() == ring;
}

Game &game()
{
	static Game instance;
	return instance;
}

}  // namespace rogue
