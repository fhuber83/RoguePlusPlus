#include "game/Game.hpp"

#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "core/Coord.hpp"
#include "core/Maybe.hpp"
#include "entities/Creature.hpp"
#include "entities/Item.hpp"
#include "entities/List.hpp"
#include "game/Id.hpp"
#include "items/ItemCatalog.hpp"
#include "items/Kinds.hpp"
#include "world/Level.hpp"

namespace rogue {

Items::Items()
{
	s_magic = items::s_magic_base;
	p_magic = items::p_magic_base;
	r_magic = items::r_magic_base;
	ws_magic = items::ws_magic_base;
	things = items::things_base;
}

std::vector<std::string>
pool_problems(const Game &g)
{
	std::vector<std::string> problems;
	auto problem = [&](std::string text) { problems.push_back(std::move(text)); };
	const Pool &pool = g.pool;
	const world::Level &level = g.level;
	int item_refs[MAXITEMS] = {};
	int creature_refs[MAXITEMS] = {};

	// Lists hold Ids; one of a free slot is counted, and reported below
	auto count_items = [&](const List<Item> &list) {
		for (ItemId id : list.ids())
			item_refs[id.slot]++;
	};
	count_items(level.objects);
	count_items(g.player.body.t_pack);
	for (CreatureId id : level.monsters.ids()) {
		int slot = id.slot;
		creature_refs[slot]++;
		Maybe<const Creature> tp = pool.creature(std::optional<CreatureId>(id));
		if (!tp)
			continue;
		count_items(tp->t_pack);

		const std::optional<Destination> &dest = tp->t_dest;
		bool dest_ok = !dest || std::holds_alternative<Hero>(*dest)
			|| (std::holds_alternative<Gold>(*dest) && world::Level::valid(std::get<Gold>(*dest).room));
		if (dest && std::holds_alternative<ItemId>(*dest)) {
			Maybe<Item> obj = pool.item(std::optional<ItemId>(std::get<ItemId>(*dest)));
			dest_ok = obj && level.objects.contains(*obj);
		}
		if (!dest_ok)
			problem("monster " + std::to_string(slot) + " is after something that isn't the hero, gold or a floor item");
		if (tp->t_room && !world::Level::valid(*tp->t_room))
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
		if (worn && (!pool.item(worn) || !player.body.t_pack.contains(*pool.item(worn))))
			problem("a worn item isn't in the pack");
	if (g.turn.last_item && !pool.item(g.turn.last_item))
		problem("the item picked last isn't in use");
	if (player.body.t_room && !world::Level::valid(*player.body.t_room))
		problem("the rogue is in a room that isn't one");
	if (player.old_room && !world::Level::valid(*player.old_room))
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

Item *ListPool<Item>::at(ItemId id) { return game().pool.items.find(id); }
std::optional<ItemId> ListPool<Item>::id_of(const Item &obj) { return game().pool.id_of(obj); }
Creature *ListPool<Creature>::at(CreatureId id) { return game().pool.creatures.find(id); }
std::optional<CreatureId> ListPool<Creature>::id_of(const Creature &tp) { return game().pool.id_of(tp); }

Maybe<Item> Player::armor_item() const { return game().pool.item(armor); }
Maybe<Item> Player::weapon_item() const { return game().pool.item(weapon); }
Maybe<Item> Player::ring_item(Hand hand) const { return game().pool.item(rings[hand]); }

bool Player::wears(Hand hand, Ring ring) const
{
	Maybe<Item> obj = ring_item(hand);
	return obj && obj->which<Ring>() == ring;
}

Game &game()
{
	static Game instance;
	return instance;
}

}  // namespace rogue
