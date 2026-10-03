/*
 * Saving and restoring games. The original (save.c) wrote the program's
 * data segment to disk; see SaveGame.hpp for what is written instead.
 */

#include "persistence/SaveGame.hpp"

#include <array>
#include <cstdio>
#include <expected>
#include <format>
#include <fstream>
#include <functional>
#include <ios>
#include <iterator>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "core/Coord.hpp"
#include "core/Dice.hpp"
#include "core/Glyphs.hpp"
#include "core/KindTable.hpp"
#include "core/Maybe.hpp"
#include "core/Random.hpp"
#include "entities/Creature.hpp"
#include "entities/Item.hpp"
#include "entities/List.hpp"
#include "entities/MonsterCatalog.hpp"
#include "entities/Stats.hpp"
#include "game/Game.hpp"
#include "game/Id.hpp"
#include "items/KindInfo.hpp"
#include "items/Kinds.hpp"
#include "items/effects/Weapon.hpp"
#include "persistence/ByteText.hpp"
#include "rules/Scheduler.hpp"
#include "ui/Display.hpp"
#include "world/Map.hpp"
#include "world/MapFlags.hpp"
#include "world/Room.hpp"
#include "world/RoomRef.hpp"

namespace rogue::persistence {

namespace {

using json = nlohmann::ordered_json;	// keeps keys in the order written

constexpr std::string_view format_name = "rogue++ save";
constexpr int format_version = 1;

static_assert(map_rows == maxrow - 1 && map_cols == MAXCOLS);

/*
 * Every field of the game is saved. A field added to one of these types
 * changes its size and stops the build here: save and load the new field,
 * then update the size (measured on x86-64 Linux, where these hold).
 */
#if defined(__x86_64__) && defined(__linux__)
static_assert(sizeof(Game) == 18560, "a Game member was added or removed: save it");
static_assert(sizeof(Player) == 328, "a Player field was added or removed: save it");
static_assert(sizeof(Level) == 6488, "a Level field was added or removed: save it");
static_assert(sizeof(Items) == 4808, "an Items field was added or removed: save it");
static_assert(sizeof(Pool) == 1336, "a Pool field was added or removed: save it");
static_assert(sizeof(Turn) == 80, "a Turn field was added or removed: save it");
static_assert(sizeof(MessageLine) == 80, "a MessageLine field was added or removed: save it");
static_assert(sizeof(Options) == 264, "an Options field was added or removed: decide whether to save it");
static_assert(sizeof(Creature) == 152, "a Creature field was added or removed: save it");
static_assert(sizeof(Item) == 128, "an Item field was added or removed: save it");
static_assert(sizeof(world::Room) == 132, "a room field was added or removed: save it");
static_assert(sizeof(entities::Stats) == 80, "a stats field was added or removed: save it");
#endif

// A wrong or missing value while loading
struct LoadError : std::runtime_error {
	using std::runtime_error::runtime_error;
};

[[noreturn]] void fail(const std::string &what)
{
	throw LoadError(what);
}

/*
 * Texts that loaded pointers point to (damage, names, typeahead). They are
 * kept for the rest of the program, as the string literals they stand for
 * were.
 */
std::string_view intern(std::string_view text)
{
	static std::set<std::string, std::less<>> texts;
	return *texts.emplace(text).first;
}

// Writing

json text_json(std::string_view text)
{
	return bytes_to_utf8(text);
}

json coord_json(const Coord &c)
{
	return json::array({c.x, c.y});
}

json item_ref(const Game &g, Maybe<const Item> obj)
{
	std::optional<ItemId> id = g.pool.id_of(obj);
	if (!id)
		return nullptr;
	return id->slot;
}

// A link to an item, null for none or a slot not in use
json item_ref(const Game &g, std::optional<ItemId> id)
{
	return item_ref(g, g.pool.item(id));
}

json room_ref(std::optional<RoomRef> ref)
{
	if (!ref)
		return nullptr;
	return json{{ref->kind == RoomRef::Kind::Room ? "room" : "passage", ref->index}};
}

json dest_ref(const Game &g, std::optional<Destination> dest)
{
	if (!dest)
		return nullptr;
	if (std::holds_alternative<Hero>(*dest))
		return "hero";
	if (std::holds_alternative<Gold>(*dest)) {
		const RoomRef room = std::get<Gold>(*dest).room;
		return json{{room.kind == RoomRef::Kind::Room ? "room_gold" : "passage_gold", room.index}};
	}
	const ItemId id = std::get<ItemId>(*dest);
	if (!g.pool.items.used(id.slot))
		throw std::logic_error("a monster is after something that can't be saved");
	return json{{"item", id.slot}};
}

// Damage as its text, null for none
json attacks_json(const Attacks &attacks)
{
	if (attacks.empty())
		return nullptr;
	return attacks.to_string();
}

// The alias every venus flytrap's damage is saved as (see flytrap_attacks())
const json flytrap_alias = {{"alias", "flytrap"}};

// A monster that fights with the flytraps' growing attack
bool is_flytrap(const Game &g, const Creature &c)
{
	return c.t_type == 'F' && &c != &g.player.body;
}

json stats_json(const entities::Stats &s, bool flytrap)
{
	return {
		{"str", s.s_str}, {"exp", s.s_exp}, {"level", s.s_lvl}, {"armor", s.s_arm},
		{"hp", s.s_hpt}, {"damage", flytrap ? flytrap_alias : attacks_json(s.s_dmg)}, {"max_hp", s.s_maxhp},
	};
}

json item_list(const Game &g, const List<Item> &list)
{
	json out = json::array();
	for (ItemId id : list.ids())
		out.push_back(item_ref(g, id));
	return out;
}

json creature_json(const Game &g, const Creature &c)
{
	return {
		{"pos", coord_json(c.t_pos)}, {"turn", c.t_turn}, {"type", c.t_type},
		{"disguise", c.t_disguise}, {"oldch", c.t_oldch}, {"dest", dest_ref(g, c.t_dest)},
		{"flags", c.t_flags.bits()}, {"stats", stats_json(c.t_stats, is_flytrap(g, c))},
		{"room", room_ref(c.t_room)}, {"pack", item_list(g, c.t_pack)},
	};
}

json item_json(const Item &o)
{
	return {
		{"kind", static_cast<int>(o.o_type)}, {"pos", coord_json(o.o_pos)},
		{"launch", o.o_launch}, {"damage", attacks_json(o.o_damage)}, {"hurl", attacks_json(o.o_hurldmg)},
		{"count", o.o_count}, {"which", o.o_which}, {"hplus", o.o_hplus}, {"dplus", o.o_dplus},
		{"ac", o.o_ac}, {"flags", o.o_flags.bits()}, {"enemy", o.o_enemy}, {"group", o.o_group},
	};
}

json room_json(const world::Room &r)
{
	json exits = json::array();
	for (const Coord &c : r.r_exit)
		exits.push_back(coord_json(c));
	return {
		{"pos", coord_json(r.r_pos)}, {"size", coord_json(r.r_max)}, {"gold", coord_json(r.r_gold)},
		{"gold_value", r.r_goldval}, {"flags", r.r_flags.bits()}, {"exit_count", r.r_nexits},
		{"exits", std::move(exits)},
	};
}

constexpr std::string_view hex_digits = "0123456789abcdef";

// The map rows of a column-major level grid (see INDEX()), as hex
// A grid is saved as its bytes: the map's glyphs, or the MapFlags' bits
unsigned char byte_of(unsigned char cell) { return cell; }
unsigned char byte_of(MapFlags cell) { return cell.bits(); }
void set_byte(unsigned char &cell, unsigned char b) { cell = b; }
void set_byte(MapFlags &cell, unsigned char b) { cell = MapFlags::from_bits(b); }

// grid: Level::map or Level::flags
json grid_json(const auto &grid)
{
	json rows = json::array();
	for (int y = 1; y < maxrow; y++) {
		std::string row;
		for (int x = 0; x < MAXCOLS; x++) {
			unsigned char b = byte_of(grid[world::INDEX(y, x)]);
			row += hex_digits[b >> 4];
			row += hex_digits[b & 0xf];
		}
		rows.push_back(std::move(row));
	}
	return rows;
}

json odds_json(std::span<const items::KindInfo> odds)
{
	json out = json::array();
	for (const items::KindInfo &mi : odds)
		out.push_back(json::array({mi.mi_prob, mi.mi_worth}));
	return out;
}

// texts and values: a KindTable or an array
json texts_json(const auto &texts)
{
	json out = json::array();
	for (const auto &t : texts)
		out.push_back(text_json(t));
	return out;
}

json bools_json(const auto &values)
{
	json out = json::array();
	for (bool v : values)
		out.push_back(v);
	return out;
}

/*
 * The guesses were buffers in one pool, handed out in the order main() calls
 * init_names(), init_colors(), init_stones() and init_materials(); the file
 * keeps that: "guesses" is the pool and "guessed" each kind's index in it.
 */
constexpr std::size_t guess_pool_size =
	kind_count<Scroll> + kind_count<Potion> + kind_count<Ring> + kind_count<Stick>;

template <KindEnum E>
json guess_refs(const KindTable<E, std::string> &guesses, json &pool)
{
	json out = json::array();
	for (const std::string &guess : guesses) {
		out.push_back(pool.size());
		pool.push_back(bytes_to_utf8(guess));
	}
	return out;
}

json items_json(const Items &items)
{
	json names = json::array();
	for (const std::string &name : items.s_names)
		names.push_back(bytes_to_utf8(name));
	json guesses = json::array();
	json guessed = {
		{"scrolls", guess_refs(items.s_guess, guesses)},
		{"potions", guess_refs(items.p_guess, guesses)},
		{"rings", guess_refs(items.r_guess, guesses)},
		{"sticks", guess_refs(items.ws_guess, guesses)},
	};
	return {
		{"odds", {
			{"scrolls", odds_json(items.s_magic)},
			{"potions", odds_json(items.p_magic)},
			{"rings", odds_json(items.r_magic)},
			{"sticks", odds_json(items.ws_magic)},
			{"things", odds_json(items.things)},
		}},
		{"scroll_names", std::move(names)},
		{"potion_colors", texts_json(items.p_colors)},
		{"ring_stones", texts_json(items.r_stones)},
		{"stick_materials", texts_json(items.ws_made)},
		{"stick_types", texts_json(items.ws_type)},
		{"known", {
			{"scrolls", bools_json(items.s_know)},
			{"potions", bools_json(items.p_know)},
			{"rings", bools_json(items.r_know)},
			{"sticks", bools_json(items.ws_know)},
		}},
		{"guesses", std::move(guesses)},
		{"guessed", std::move(guessed)},
		{"next_guess", guess_pool_size},
		{"group", items.group},
	};
}

json player_json(const Game &g)
{
	const Player &p = g.player;
	return {
		{"body", creature_json(g, p.body)},
		{"max_stats", stats_json(p.max_stats, false)},
		{"purse", p.purse}, {"in_pack", p.in_pack},
		{"armor", item_ref(g, p.armor)}, {"weapon", item_ref(g, p.weapon)},
		{"rings", json::array({item_ref(g, p.rings[Hand::Left]), item_ref(g, p.rings[Hand::Right])})},
		{"food_left", p.food_left}, {"hungry_state", p.hungry_state},
		{"has_amulet", p.has_amulet}, {"saw_amulet", p.saw_amulet},
		{"max_level", p.max_level}, {"no_command", p.no_command}, {"no_move", p.no_move},
		{"quiet", p.quiet}, {"fungus_hits", p.fung_hit},
		{"flytrap_damage", entities::flytrap_attacks(p.fung_hit).to_string()},
		{"was_trapped", std::to_underlying(p.was_trapped)},
		{"old_pos", coord_json(p.old_pos)}, {"old_room", room_ref(p.old_room)},
	};
}

json level_json(const Game &g)
{
	const Level &l = g.level;
	json rooms = json::array(), passages = json::array(), monsters = json::array();
	for (const world::Room &r : l.rooms)
		rooms.push_back(room_json(r));
	for (const world::Room &r : l.passages)
		passages.push_back(room_json(r));
	for (CreatureId id : l.monsters.ids())
		monsters.push_back(g.pool.creatures.used(id.slot) ? id.slot : -1);
	return {
		{"depth", l.depth}, {"traps", l.ntraps}, {"no_food", l.no_food},
		{"rooms", std::move(rooms)}, {"passages", std::move(passages)},
		{"map", grid_json(l.map)}, {"flags", grid_json(l.flags)},
		{"objects", item_list(g, l.objects)}, {"monsters", std::move(monsters)},
	};
}

json pool_json(const Game &g)
{
	json items = json::array(), creatures = json::array();
	for (int i = 0; i < MAXITEMS; i++) {
		if (const Item *obj = g.pool.items.at(i)) {
			json o = {{"slot", i}};
			o.update(item_json(*obj));
			items.push_back(std::move(o));
		}
		if (const Creature *tp = g.pool.creatures.at(i)) {
			json c = {{"slot", i}};
			c.update(creature_json(g, *tp));
			creatures.push_back(std::move(c));
		}
	}
	return {{"items", std::move(items)}, {"creatures", std::move(creatures)}};
}

json turn_json(const Game &g)
{
	const Turn &t = g.turn;
	return {
		{"after", t.after}, {"again", t.again}, {"count", t.count}, {"take", t.take},
		{"running", t.running}, {"run_dir", t.run_dir}, {"door_stop", t.door_stop},
		{"first_move", t.first_move}, {"fast_mode", t.fast_mode}, {"fast_state", t.fast_state},
		{"delta", coord_json(t.delta)}, {"typeahead", text_json(t.typeahead)},
		{"bailout", t.bailout}, {"moves_left", t.moves_left}, {"last_count", t.last_count}, {"last_ch", t.last_ch},
		{"last_take", t.last_take}, {"do_take", t.do_take},
		{"last_item_key", t.last_item_key}, {"last_item", item_ref(g, t.last_item)},
	};
}

json screen_json(const MapView &view)
{
	json glyphs = json::array(), styles = json::array();
	for (const auto &row : view) {
		std::string g, s;
		for (const MapTile &tile : row) {
			g += hex_digits[tile.glyph >> 4];
			g += hex_digits[tile.glyph & 0xf];
			s += static_cast<char>('0' + static_cast<int>(tile.style));
		}
		glyphs.push_back(std::move(g));
		styles.push_back(std::move(s));
	}
	return {{"glyphs", std::move(glyphs)}, {"styles", std::move(styles)}};
}

// Reading

const json &field(const json &j, std::string_view key)
{
	if (!j.is_object())
		fail(std::format("expected an object holding \"{}\"", key));
	auto it = j.find(key);
	if (it == j.end())
		fail(std::format("\"{}\" is missing", key));
	return *it;
}

// std::in_range() takes no plain char; check it as the char type it is
template <class T>
using RangeOf = std::conditional_t<std::is_same_v<T, char>,
	std::conditional_t<std::is_signed_v<char>, signed char, unsigned char>, T>;

template <class T>
T num(const json &j, std::string_view key)
{
	const json &v = field(j, key);
	if (!v.is_number_integer())
		fail(std::format("\"{}\" is not a whole number", key));
	if (v.is_number_unsigned()) {
		auto u = v.get<unsigned long long>();
		if (!std::in_range<RangeOf<T>>(u))
			fail(std::format("\"{}\" is out of range", key));
		return static_cast<T>(u);
	}
	auto n = v.get<long long>();
	if (!std::in_range<RangeOf<T>>(n))
		fail(std::format("\"{}\" is out of range", key));
	return static_cast<T>(n);
}

template <class T>
T num_in(const json &j, std::string_view key, T lo, T hi)
{
	T n = num<T>(j, key);
	if (n < lo || n > hi)
		fail(std::format("\"{}\" is out of range", key));
	return n;
}

bool flag(const json &j, std::string_view key)
{
	const json &v = field(j, key);
	if (!v.is_boolean())
		fail(std::format("\"{}\" is not true or false", key));
	return v.get<bool>();
}

const json &array_of(const json &j, std::string_view key, std::size_t size)
{
	const json &v = field(j, key);
	if (!v.is_array() || v.size() != size)
		fail(std::format("\"{}\" should be a list of {}", key, size));
	return v;
}

int whole(const json &v, std::string_view what)
{
	if (!v.is_number_integer())
		fail(std::format("{} is not a whole number", what));
	auto n = v.get<long long>();
	if (!std::in_range<int>(n))
		fail(std::format("{} is out of range", what));
	return static_cast<int>(n);
}

Coord to_coord(const json &v, std::string_view what)
{
	if (!v.is_array() || v.size() != 2)
		fail(std::format("{} should be [x, y]", what));
	return Coord{whole(v[0], what), whole(v[1], what)};
}

Coord coord_of(const json &j, std::string_view key)
{
	return to_coord(field(j, key), key);
}

// Text into a char buffer of the given size (with its NUL)
// Text of at most max bytes
std::string text_of(const json &j, std::string_view key, std::size_t max = std::string::npos)
{
	const json &v = field(j, key);
	if (!v.is_string())
		fail(std::format("\"{}\" is not text", key));
	std::string bytes = utf8_to_bytes(v.get<std::string>());
	if (bytes.size() > max)
		fail(std::format("\"{}\" is too long", key));
	return bytes;
}

// Text kept for the rest of the program, "" for null
std::string_view kept_text(const json &v, std::string_view what)
{
	if (v.is_null())
		return "";
	if (!v.is_string())
		fail(std::format("{} is not text", what));
	return intern(utf8_to_bytes(v.get<std::string>()));
}

/*
 * A slot number, or nothing for null. With used, the slot must be in use;
 * without, a free slot gives nothing (a save made before discard() forgot
 * the last item picked can name the freed slot).
 */
Maybe<Item> item_at(Game &g, const json &v, std::string_view what, bool used = true)
{
	if (v.is_null())
		return std::nullopt;
	int slot = whole(v, what);
	Maybe<Item> obj = maybe(g.pool.items.at(slot));
	if (slot < 0 || slot >= MAXITEMS || (used && !obj))
		fail(std::format("{} is not an item in use", what));
	return obj;
}

std::optional<RoomRef> room_at(const json &v, std::string_view what)
{
	if (v.is_null())
		return std::nullopt;
	if (v.is_object() && v.size() == 1) {
		std::optional<RoomRef> ref;
		if (v.contains("room"))
			ref = RoomRef::room(whole(v["room"], what));
		else if (v.contains("passage"))
			ref = RoomRef::passage(whole(v["passage"], what));
		if (ref && Level::valid(*ref))
			return ref;
	}
	fail(std::format("{} is not a room or passage", what));
}

std::optional<Destination> dest_at(Game &g, const json &v)
{
	if (v.is_null())
		return std::nullopt;
	if (v == "hero")
		return Hero{};
	if (v.is_object() && v.size() == 1) {
		std::optional<RoomRef> gold;
		if (v.contains("room_gold"))
			gold = RoomRef::room(whole(v["room_gold"], "dest"));
		else if (v.contains("passage_gold"))
			gold = RoomRef::passage(whole(v["passage_gold"], "dest"));
		else if (v.contains("item"))
			return *g.pool.id_of(item_at(g, v["item"], "dest"));
		if (gold && Level::valid(*gold))
			return Gold{*gold};
	}
	fail("\"dest\" is not the hero, gold or an item");
}

// Damage from its text, none for null
Attacks attacks_of(const json &v, std::string_view what)
{
	if (v.is_null())
		return {};
	if (!v.is_string())
		fail(std::format("{} is not text", what));
	auto attacks = Attacks::parse(utf8_to_bytes(v.get<std::string>()));
	if (!attacks)
		fail(std::format("{} is not damage like \"1d2/1d5\"", what));
	return *attacks;
}

entities::Stats stats_from(const json &j, bool flytrap)
{
	entities::Stats s{};
	s.s_str = num<entities::str_t>(j, "str");
	s.s_exp = num<long>(j, "exp");
	s.s_lvl = num<int>(j, "level");
	s.s_arm = num<int>(j, "armor");
	s.s_hpt = num<int>(j, "hp");
	const json &damage = field(j, "damage");
	if (flytrap != (damage == flytrap_alias))
		fail(flytrap ? "a venus flytrap's \"damage\" is not the flytrap alias"
			: "only a venus flytrap's \"damage\" is the flytrap alias");
	s.s_dmg = flytrap ? entities::monsters['F'-'A'].m_stats.s_dmg : attacks_of(damage, "\"damage\"");
	s.s_maxhp = num<int>(j, "max_hp");
	return s;
}

void items_into(Game &g, List<Item> &list, const json &slots, std::string_view what)
{
	if (!slots.is_array())
		fail(std::format("{} is not a list", what));
	// Kept in their order: push each to the back
	Maybe<const Item> last;
	for (const json &v : slots) {
		Maybe<Item> obj = item_at(g, v, what);
		if (!obj)
			fail(std::format("{} holds a null", what));
		list.insert_after(last, *obj);
		last = obj;
	}
}

void creature_from(Game &g, Creature &c, const json &j)
{
	c.t_pos = coord_of(j, "pos");
	c.t_turn = num<char>(j, "turn");
	c.t_type = num<char>(j, "type");
	c.t_disguise = num<unsigned char>(j, "disguise");
	c.t_oldch = num<unsigned char>(j, "oldch");
	c.t_dest = dest_at(g, field(j, "dest"));
	c.t_flags = CreatureFlags::from_bits(num<CreatureFlags::Bits>(j, "flags"));
	c.t_stats = stats_from(field(j, "stats"), is_flytrap(g, c));
	c.t_room = room_at(field(j, "room"), "\"room\"");
	items_into(g, c.t_pack, field(j, "pack"), "\"pack\"");
}

void item_from(Item &o, const json &j)
{
	o.o_type = static_cast<ItemKind>(num_in<int>(j, "kind", 0, static_cast<int>(ItemKind::Missile)));
	o.o_pos = coord_of(j, "pos");
	o.o_launch = num<char>(j, "launch");
	o.o_damage = attacks_of(field(j, "damage"), "\"damage\"");
	o.o_hurldmg = attacks_of(field(j, "hurl"), "\"hurl\"");
	o.o_count = num<int>(j, "count");
	o.o_which = num<int>(j, "which");
	o.o_hplus = num<int>(j, "hplus");
	o.o_dplus = num<int>(j, "dplus");
	o.o_ac = num<short>(j, "ac");
	o.o_flags = ItemFlags::from_bits(num<ItemFlags::Bits>(j, "flags"));
	o.o_enemy = num<char>(j, "enemy");
	o.o_group = num<int>(j, "group");
}

void room_from(world::Room &r, const json &j)
{
	r.r_pos = coord_of(j, "pos");
	r.r_max = coord_of(j, "size");
	r.r_gold = coord_of(j, "gold");
	r.r_goldval = num<int>(j, "gold_value");
	r.r_flags = RoomFlags::from_bits(num<RoomFlags::Bits>(j, "flags"));
	r.r_nexits = num_in<int>(j, "exit_count", 0, std::size(r.r_exit));
	const json &exits = array_of(j, "exits", std::size(r.r_exit));
	for (std::size_t i = 0; i < std::size(r.r_exit); i++)
		r.r_exit[i] = to_coord(exits[i], "\"exits\"");
}

int hex_value(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	return -1;
}

// A row of MAXCOLS bytes as hex
std::vector<unsigned char> hex_row(const json &v, std::string_view what)
{
	if (!v.is_string() || v.get_ref<const std::string &>().size() != 2 * MAXCOLS)
		fail(std::format("{} rows should be {} hex digits", what, 2 * MAXCOLS));
	const std::string &s = v.get_ref<const std::string &>();
	std::vector<unsigned char> out;
	for (int x = 0; x < MAXCOLS; x++) {
		int hi = hex_value(s[2 * x]), lo = hex_value(s[2 * x + 1]);
		if (hi < 0 || lo < 0)
			fail(std::format("{} rows should be hex digits", what));
		out.push_back(static_cast<unsigned char>(hi << 4 | lo));
	}
	return out;
}

void grid_from(auto &grid, const json &j, std::string_view key)
{
	const json &rows = array_of(j, key, map_rows);
	for (int y = 1; y < maxrow; y++) {
		std::vector<unsigned char> row = hex_row(rows[y - 1], key);
		for (int x = 0; x < MAXCOLS; x++)
			set_byte(grid[world::INDEX(y, x)], row[x]);
	}
}

void odds_from(std::span<items::KindInfo> odds, const json &j, std::string_view key)
{
	const json &list = array_of(j, key, odds.size());
	for (std::size_t i = 0; i < odds.size(); i++) {
		if (!list[i].is_array() || list[i].size() != 2)
			fail(std::format("\"{}\" entries should be [odds, worth]", key));
		odds[i].mi_prob = whole(list[i][0], key);
		int worth = whole(list[i][1], key);
		if (!std::in_range<short>(worth))
			fail(std::format("\"{}\" is out of range", key));
		odds[i].mi_worth = static_cast<short>(worth);
	}
}

template <KindEnum E>
void kept_texts_from(KindTable<E, std::string_view> &texts, const json &j, std::string_view key)
{
	const json &list = array_of(j, key, texts.size());
	for (std::size_t i = 0; i < texts.size(); i++) {
		texts.data()[i] = kept_text(list[i], key);
	}
}

template <KindEnum E>
void bools_from(KindTable<E, bool> &values, const json &j, std::string_view key)
{
	const json &list = array_of(j, key, values.size());
	for (std::size_t i = 0; i < values.size(); i++) {
		if (!list[i].is_boolean())
			fail(std::format("\"{}\" should hold true or false", key));
		values.data()[i] = list[i].get<bool>();
	}
}

template <KindEnum E>
void guess_refs_from(KindTable<E, std::string> &guesses, const std::vector<std::string> &pool,
	const json &j, std::string_view key)
{
	const json &list = array_of(j, key, guesses.size());
	for (std::size_t i = 0; i < guesses.size(); i++) {
		if (list[i].is_null()) {			/* a game saved before init_*() */
			guesses.data()[i].clear();
			continue;
		}
		int n = whole(list[i], key);
		if (n < 0 || n >= static_cast<int>(pool.size()))
			fail(std::format("\"{}\" is out of range", key));
		guesses.data()[i] = pool[n];
	}
}

void items_from(Items &items, const json &j)
{
	const json &odds = field(j, "odds");
	odds_from(items.s_magic, odds, "scrolls");
	odds_from(items.p_magic, odds, "potions");
	odds_from(items.r_magic, odds, "rings");
	odds_from(items.ws_magic, odds, "sticks");
	odds_from(items.things, odds, "things");

	const json &names = array_of(j, "scroll_names", items.s_names.size());
	for (std::size_t i = 0; i < items.s_names.size(); i++)
		items.s_names.data()[i] = text_of(json{{"name", names[i]}}, "name", MAXNAME);
	kept_texts_from(items.p_colors, j, "potion_colors");
	kept_texts_from(items.r_stones, j, "ring_stones");
	kept_texts_from(items.ws_made, j, "stick_materials");
	kept_texts_from(items.ws_type, j, "stick_types");

	const json &known = field(j, "known");
	bools_from(items.s_know, known, "scrolls");
	bools_from(items.p_know, known, "potions");
	bools_from(items.r_know, known, "rings");
	bools_from(items.ws_know, known, "sticks");

	const json &texts = array_of(j, "guesses", guess_pool_size);
	std::vector<std::string> pool;
	for (const json &text : texts)
		pool.push_back(text_of(json{{"guess", text}}, "guess", MAXNAME));
	const json &guessed = field(j, "guessed");
	guess_refs_from(items.s_guess, pool, guessed, "scrolls");
	guess_refs_from(items.p_guess, pool, guessed, "potions");
	guess_refs_from(items.r_guess, pool, guessed, "rings");
	guess_refs_from(items.ws_guess, pool, guessed, "sticks");
	num_in<int>(j, "next_guess", 0, guess_pool_size);	// the pool's use, no longer needed
	items.group = num<int>(j, "group");
}

void pool_from(Game &g, const json &j)
{
	// Mark every slot first: packs and destinations refer to other slots
	const json &items = field(j, "items");
	const json &creatures = field(j, "creatures");
	if (!items.is_array() || !creatures.is_array())
		fail("\"pool\" should list items and creatures");
	auto claim = [&](const json &entry, auto &slots) {
		int slot = num_in<int>(entry, "slot", 0, MAXITEMS - 1);
		auto *thing = slots.take_at(slot);
		if (thing == nullptr)
			fail("pool slot " + std::to_string(slot) + " is saved twice");
		g.pool.total++;
		return thing;
	};
	std::vector<Item *> item_slots;
	std::vector<Creature *> creature_slots;
	for (const json &entry : items)
		item_slots.push_back(claim(entry, g.pool.items));
	for (const json &entry : creatures)
		creature_slots.push_back(claim(entry, g.pool.creatures));
	for (std::size_t i = 0; i < items.size(); i++)
		item_from(*item_slots[i], items[i]);
	for (std::size_t i = 0; i < creatures.size(); i++)
		creature_from(g, *creature_slots[i], creatures[i]);
}

void level_from(Game &g, const json &j)
{
	Level &l = g.level;
	l.depth = num<int>(j, "depth");
	l.ntraps = num<int>(j, "traps");
	l.no_food = num<int>(j, "no_food");
	const json &rooms = array_of(j, "rooms", world::MAXROOMS);
	for (int i = 0; i < world::MAXROOMS; i++)
		room_from(l.rooms[i], rooms[i]);
	const json &passages = array_of(j, "passages", world::MAXPASS);
	for (int i = 0; i < world::MAXPASS; i++)
		room_from(l.passages[i], passages[i]);
	grid_from(l.map, j, "map");
	grid_from(l.flags, j, "flags");
	items_into(g, l.objects, field(j, "objects"), "\"objects\"");

	const json &monsters = field(j, "monsters");
	if (!monsters.is_array())
		fail("\"monsters\" is not a list");
	Maybe<const Creature> last;
	for (const json &v : monsters) {
		int slot = whole(v, "\"monsters\"");
		Maybe<Creature> tp = maybe(g.pool.creatures.at(slot));
		if (!tp)
			fail("\"monsters\" holds a creature not in use");
		l.monsters.insert_after(last, *tp);
		last = tp;
	}
}

void player_from(Game &g, const json &j)
{
	Player &p = g.player;
	creature_from(g, p.body, field(j, "body"));
	p.max_stats = stats_from(field(j, "max_stats"), false);
	p.purse = num<int>(j, "purse");
	p.in_pack = num<int>(j, "in_pack");
	p.armor = g.pool.id_of(item_at(g, field(j, "armor"), "\"armor\""));
	p.weapon = g.pool.id_of(item_at(g, field(j, "weapon"), "\"weapon\""));
	const json &rings = array_of(j, "rings", 2);
	p.rings[Hand::Left] = g.pool.id_of(item_at(g, rings[0], "\"rings\""));
	p.rings[Hand::Right] = g.pool.id_of(item_at(g, rings[1], "\"rings\""));
	p.food_left = num<int>(j, "food_left");
	p.hungry_state = num<int>(j, "hungry_state");
	p.has_amulet = flag(j, "has_amulet");
	p.saw_amulet = flag(j, "saw_amulet");
	p.max_level = num<int>(j, "max_level");
	p.no_command = num<int>(j, "no_command");
	p.no_move = num<int>(j, "no_move");
	p.quiet = num<int>(j, "quiet");
	p.fung_hit = num<int>(j, "fungus_hits");
	if (text_of(j, "flytrap_damage") != entities::flytrap_attacks(p.fung_hit).to_string())
		fail("\"flytrap_damage\" does not follow from \"fungus_hits\"");
	p.was_trapped = static_cast<Trapped>(num_in<unsigned char>(j, "was_trapped", 0, 2));
	p.old_pos = coord_of(j, "old_pos");
	p.old_room = room_at(field(j, "old_room"), "\"old_room\"");
}

void turn_from(Game &g, const json &j)
{
	Turn &t = g.turn;
	t.after = flag(j, "after");
	t.again = flag(j, "again");
	t.count = num<int>(j, "count");
	t.take = num<char>(j, "take");
	t.running = flag(j, "running");
	t.run_dir = num<char>(j, "run_dir");
	t.door_stop = flag(j, "door_stop");
	t.first_move = flag(j, "first_move");
	t.fast_mode = flag(j, "fast_mode");
	t.fast_state = flag(j, "fast_state");
	t.delta = coord_of(j, "delta");
	t.typeahead = text_of(j, "typeahead");
	t.bailout = flag(j, "bailout");
	// A save made before F.2 has none: it goes on with one move, as restoring it did then
	t.moves_left = j.contains("moves_left") ? num_in<int>(j, "moves_left", 0, 3) : 1;
	t.last_count = num<int>(j, "last_count");
	t.last_ch = num<unsigned char>(j, "last_ch");
	t.last_take = num<unsigned char>(j, "last_take");
	t.do_take = num<unsigned char>(j, "do_take");
	t.last_item_key = num<unsigned char>(j, "last_item_key");
	t.last_item = g.pool.id_of(item_at(g, field(j, "last_item"), "\"last_item\"", false));
}

void screen_from(MapView &view, const json &j)
{
	const json &glyphs = array_of(j, "glyphs", map_rows);
	const json &styles = array_of(j, "styles", map_rows);
	for (int r = 0; r < map_rows; r++) {
		std::vector<unsigned char> row = hex_row(glyphs[r], "\"glyphs\"");
		if (!styles[r].is_string() || styles[r].get_ref<const std::string &>().size() != map_cols)
			fail("\"styles\" rows should be " + std::to_string(map_cols) + " digits");
		const std::string &s = styles[r].get_ref<const std::string &>();
		for (int x = 0; x < map_cols; x++) {
			if (s[x] < '0' || s[x] > '0' + static_cast<int>(ui::TileStyle::FrostBolt))
				fail("\"styles\" holds an unknown style");
			view[r][x] = MapTile{row[x], static_cast<ui::TileStyle>(s[x] - '0')};
		}
	}
}

void game_from(Game &g, MapView &view, const json &doc)
{
	const json &random = field(doc, "random");
	const json &state = field(random, "state");
	if (!state.is_string() || !g.random.restore(num<Random::Seed>(random, "seed"), state.get<std::string>()))
		fail("\"random\" is not a generator state");

	const json &options = field(doc, "options");
	g.options.name = text_of(options, "name", Options::name_length);
	g.options.fruit = text_of(options, "fruit", Options::name_length);
	g.options.terse = flag(options, "terse");
	g.options.expert = flag(options, "expert");

	g.pool = Pool();
	g.level = Level();
	g.player = Player();
	g.items = Items();
	g.scheduler = rules::Scheduler();
	g.message = MessageLine();
	g.turn = Turn();

	pool_from(g, field(doc, "pool"));
	level_from(g, field(doc, "level"));
	player_from(g, field(doc, "player"));
	items_from(g.items, field(doc, "items"));

	const json &slots = array_of(doc, "scheduler", rules::Scheduler::max_actions);
	std::array<rules::Scheduler::Slot, rules::Scheduler::max_actions> table;
	for (int i = 0; i < rules::Scheduler::max_actions; i++) {
		const json &slot = slots[i];
		if (!slot.is_array() || slot.size() != 2)
			fail("\"scheduler\" entries should be [event, time]");
		int event = whole(slot[0], "\"scheduler\"");
		if (event < 0 || event > static_cast<int>(rules::Event::TurnSeeOff))
			fail("\"scheduler\" holds an unknown event");
		table[i] = {static_cast<rules::Event>(event), whole(slot[1], "\"scheduler\"")};
	}
	g.scheduler.set_slots(table);

	const json &message = field(doc, "message");
	g.message.text = text_of(message, "text", BUFSIZE - 1);
	g.message.last = text_of(message, "last", BUFSIZE - 1);
	g.message.end = num_in<int>(message, "end", 0, MAXCOLS);
	g.message.next_end = num_in<int>(message, "next_end", 0, BUFSIZE);
	g.message.remember = flag(message, "remember");

	turn_from(g, field(doc, "turn"));
	g.playing = flag(doc, "playing");
	g.noscore = flag(doc, "noscore");
	g.wander_rolls = num<int>(doc, "wander_rolls");
	screen_from(view, field(doc, "screen"));
}

}  // namespace

std::string
format_save(const Game &g, const MapView &view)
{
	json scheduler = json::array();
	for (const rules::Scheduler::Slot &slot : g.scheduler.slots())
		scheduler.push_back(json::array({static_cast<int>(slot.event), slot.time}));

	json doc = {
		{"format", format_name},
		{"version", format_version},
		{"random", {{"seed", g.random.seed()}, {"state", g.random.state()}}},
		{"options", {
			{"name", bytes_to_utf8(g.options.name)}, {"fruit", bytes_to_utf8(g.options.fruit)},
			{"terse", g.options.terse}, {"expert", g.options.expert},
		}},
		{"pool", pool_json(g)},
		{"level", level_json(g)},
		{"player", player_json(g)},
		{"items", items_json(g.items)},
		{"scheduler", std::move(scheduler)},
		{"message", {
			{"text", bytes_to_utf8(g.message.text)}, {"last", bytes_to_utf8(g.message.last)},
			{"end", g.message.end}, {"next_end", g.message.next_end}, {"remember", g.message.remember},
		}},
		{"turn", turn_json(g)},
		{"playing", g.playing},
		{"noscore", g.noscore},
		{"wander_rolls", g.wander_rolls},
		{"screen", screen_json(view)},
	};
	return doc.dump(1, '\t') + "\n";
}

std::expected<void, SaveError>
parse_save(std::string_view text, Game &g, MapView &view)
{
	json doc = json::parse(text, nullptr, false);
	if (doc.is_discarded() || !doc.is_object())
		return std::unexpected(SaveError{SaveError::Kind::BadFormat, "not JSON"});
	auto format = doc.find("format");
	if (format == doc.end() || *format != format_name)
		return std::unexpected(SaveError{SaveError::Kind::BadFormat, "not a saved game"});
	auto version = doc.find("version");
	if (version == doc.end() || *version != format_version)
		return std::unexpected(SaveError{SaveError::Kind::WrongVersion,
			"a save of another version (this game reads version " + std::to_string(format_version) + ")"});
	try {
		game_from(g, view, doc);
	} catch (const LoadError &e) {
		return std::unexpected(SaveError{SaveError::Kind::BadFormat, e.what()});
	}
	if (auto problems = pool_problems(g); !problems.empty())
		return std::unexpected(SaveError{SaveError::Kind::Inconsistent, problems.front()});
	return {};
}

std::expected<void, SaveError>
write_save(const std::string &path, const Game &g, const MapView &view)
{
	std::string temp = path + ".tmp";
	{
		std::ofstream file(temp, std::ios::binary | std::ios::trunc);
		file << format_save(g, view);
		file.close();
		if (!file) {
			std::remove(temp.c_str());
			return std::unexpected(SaveError{SaveError::Kind::Unreadable, std::string("can't write ") + temp});
		}
	}
	if (std::rename(temp.c_str(), path.c_str()) != 0) {
		std::remove(temp.c_str());
		return std::unexpected(SaveError{SaveError::Kind::Unreadable, std::string("can't write ") + path});
	}
	return {};
}

std::expected<void, SaveError>
read_save(const std::string &path, Game &g, MapView &view)
{
	std::ifstream file(path, std::ios::binary);
	if (!file)
		return std::unexpected(SaveError{SaveError::Kind::Unreadable, std::string("can't open ") + path});
	std::string text{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
	if (file.bad())
		return std::unexpected(SaveError{SaveError::Kind::Unreadable, std::string("can't read ") + path});
	return parse_save(text, g, view);
}

}  // namespace rogue::persistence
