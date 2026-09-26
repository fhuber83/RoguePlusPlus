/*
 * Saving and restoring games. The original (save.c) wrote the program's
 * data segment to disk; see SaveGame.hpp for what is written instead.
 */

#include <cstdio>
#include <fstream>
#include <functional>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "persistence/ByteText.hpp"
#include "persistence/SaveGame.hpp"
#include "rogue.h"

namespace rogue::persistence {

namespace {

using json = nlohmann::ordered_json;	// keeps keys in the order written

constexpr std::string_view format_name = "rogue++ save";
constexpr int format_version = 1;

static_assert(map_rows == maxrow - 1 && map_cols == COLS);

/*
 * Every field of the game is saved. A field added to one of these types
 * changes its size and stops the build here: save and load the new field,
 * then update the size (measured on x86-64 Linux, where these hold).
 */
#if defined(__x86_64__) && defined(__linux__)
static_assert(sizeof(Game) == 17256, "a Game member was added or removed: save it");
static_assert(sizeof(Player) == 264, "a Player field was added or removed: save it");
static_assert(sizeof(Level) == 6488, "a Level field was added or removed: save it");
static_assert(sizeof(Items) == 3520, "an Items field was added or removed: save it");
static_assert(sizeof(Pool) == 1336, "a Pool field was added or removed: save it");
static_assert(sizeof(Turn) == 56, "a Turn field was added or removed: save it");
static_assert(sizeof(MessageLine) == 268, "a MessageLine field was added or removed: save it");
static_assert(sizeof(Options) == 137, "an Options field was added or removed: decide whether to save it");
static_assert(sizeof(Creature) == 112, "a Creature field was added or removed: save it");
static_assert(sizeof(Item) == 80, "an Item field was added or removed: save it");
static_assert(sizeof(struct room) == 132, "a room field was added or removed: save it");
static_assert(sizeof(struct stats) == 48, "a stats field was added or removed: save it");
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
const char *intern(std::string_view text)
{
	static std::set<std::string, std::less<>> texts;
	return texts.emplace(text).first->c_str();
}

template <class T>
bool points_into(const T *p, const T *first, std::size_t n)
{
	std::less<const T *> less;
	return p != nullptr && !less(p, first) && less(p, first + n);
}

// Writing

json text_json(const char *text)
{
	if (text == nullptr)
		return nullptr;
	return bytes_to_utf8(text);
}

json coord_json(const coord &c)
{
	return json::array({c.x, c.y});
}

json item_ref(const Game &g, const Item *obj)
{
	int slot = g.pool.items.slot_of(obj);
	if (slot < 0)
		return nullptr;
	return slot;
}

json room_ref(const Game &g, const struct room *rp)
{
	if (points_into(rp, g.level.rooms, MAXROOMS))
		return json{{"room", rp - g.level.rooms}};
	if (points_into(rp, g.level.passages, MAXPASS))
		return json{{"passage", rp - g.level.passages}};
	return nullptr;
}

json dest_ref(const Game &g, const coord *dest)
{
	if (dest == nullptr)
		return nullptr;
	if (dest == &g.player.body.t_pos)
		return "hero";
	for (int i = 0; i < MAXROOMS; i++)
		if (dest == &g.level.rooms[i].r_gold)
			return json{{"room_gold", i}};
	for (int i = 0; i < MAXPASS; i++)
		if (dest == &g.level.passages[i].r_gold)
			return json{{"passage_gold", i}};
	for (int i = 0; i < MAXITEMS; i++)
		if (const Item *obj = g.pool.items.at(i); obj != nullptr && dest == &obj->o_pos)
			return json{{"item", i}};
	throw std::logic_error("a monster is after something that can't be saved");
}

json damage_json(const Game &g, const char *dmg)
{
	if (dmg == g.player.flytrap_damage)
		return json{{"alias", "flytrap"}};
	return text_json(dmg);
}

json stats_json(const Game &g, const struct stats &s)
{
	return {
		{"str", s.s_str}, {"exp", s.s_exp}, {"level", s.s_lvl}, {"armor", s.s_arm},
		{"hp", s.s_hpt}, {"damage", damage_json(g, s.s_dmg)}, {"max_hp", s.s_maxhp},
	};
}

json item_list(const Game &g, const List<Item> &list)
{
	json out = json::array();
	for (const Item *obj : list)
		out.push_back(item_ref(g, obj));
	return out;
}

json creature_json(const Game &g, const Creature &c)
{
	return {
		{"pos", coord_json(c.t_pos)}, {"turn", c.t_turn}, {"type", c.t_type},
		{"disguise", c.t_disguise}, {"oldch", c.t_oldch}, {"dest", dest_ref(g, c.t_dest)},
		{"flags", c.t_flags.bits()}, {"stats", stats_json(g, c.t_stats)},
		{"room", room_ref(g, c.t_room)}, {"pack", item_list(g, c.t_pack)},
	};
}

json item_json(const Item &o)
{
	return {
		{"kind", static_cast<int>(o.o_type)}, {"pos", coord_json(o.o_pos)},
		{"launch", o.o_launch}, {"damage", text_json(o.o_damage)}, {"hurl", text_json(o.o_hurldmg)},
		{"count", o.o_count}, {"which", o.o_which}, {"hplus", o.o_hplus}, {"dplus", o.o_dplus},
		{"ac", o.o_ac}, {"flags", o.o_flags.bits()}, {"enemy", o.o_enemy}, {"group", o.o_group},
	};
}

json room_json(const struct room &r)
{
	json exits = json::array();
	for (const coord &c : r.r_exit)
		exits.push_back(coord_json(c));
	return {
		{"pos", coord_json(r.r_pos)}, {"size", coord_json(r.r_max)}, {"gold", coord_json(r.r_gold)},
		{"gold_value", r.r_goldval}, {"flags", r.r_flags.bits()}, {"exit_count", r.r_nexits},
		{"exits", std::move(exits)},
	};
}

const char hex_digits[] = "0123456789abcdef";

// The map rows of a column-major level grid (see INDEX()), as hex
json grid_json(const unsigned char *grid)
{
	json rows = json::array();
	for (int y = 1; y < maxrow; y++) {
		std::string row;
		for (int x = 0; x < COLS; x++) {
			unsigned char b = grid[INDEX(y, x)];
			row += hex_digits[b >> 4];
			row += hex_digits[b & 0xf];
		}
		rows.push_back(std::move(row));
	}
	return rows;
}

json odds_json(const struct magic_item *items, int n)
{
	json out = json::array();
	for (int i = 0; i < n; i++)
		out.push_back(json::array({items[i].mi_prob, items[i].mi_worth}));
	return out;
}

template <class T, std::size_t N>
json texts_json(const T (&texts)[N])
{
	json out = json::array();
	for (const auto &t : texts)
		out.push_back(text_json(t));
	return out;
}

template <std::size_t N>
json bools_json(const bool (&values)[N])
{
	json out = json::array();
	for (bool v : values)
		out.push_back(v);
	return out;
}

template <std::size_t N>
json guess_refs(const Items &items, char *const (&guesses)[N])
{
	json out = json::array();
	for (const char *guess : guesses) {
		const struct array *slot = reinterpret_cast<const struct array *>(guess);
		if (points_into(slot, items.guesses, std::size(items.guesses)))
			out.push_back(slot - items.guesses);
		else
			out.push_back(nullptr);
	}
	return out;
}

json items_json(const Items &items)
{
	json names = json::array();
	for (const struct array &a : items.s_names)
		names.push_back(bytes_to_utf8(a.storage));
	json guesses = json::array();
	for (const struct array &a : items.guesses)
		guesses.push_back(bytes_to_utf8(a.storage));
	return {
		{"odds", {
			{"scrolls", odds_json(items.s_magic, MAXSCROLLS)},
			{"potions", odds_json(items.p_magic, MAXPOTIONS)},
			{"rings", odds_json(items.r_magic, MAXRINGS)},
			{"sticks", odds_json(items.ws_magic, MAXSTICKS)},
			{"things", odds_json(items.things, NUMTHINGS)},
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
		{"guessed", {
			{"scrolls", guess_refs(items, items.s_guess)},
			{"potions", guess_refs(items, items.p_guess)},
			{"rings", guess_refs(items, items.r_guess)},
			{"sticks", guess_refs(items, items.ws_guess)},
		}},
		{"next_guess", items.iguess},
		{"group", items.group},
	};
}

json player_json(const Game &g)
{
	const Player &p = g.player;
	return {
		{"body", creature_json(g, p.body)},
		{"max_stats", stats_json(g, p.max_stats)},
		{"purse", p.purse}, {"in_pack", p.in_pack},
		{"armor", item_ref(g, p.armor)}, {"weapon", item_ref(g, p.weapon)},
		{"rings", json::array({item_ref(g, p.rings[0]), item_ref(g, p.rings[1])})},
		{"food_left", p.food_left}, {"hungry_state", p.hungry_state},
		{"has_amulet", p.has_amulet}, {"saw_amulet", p.saw_amulet},
		{"max_level", p.max_level}, {"no_command", p.no_command}, {"no_move", p.no_move},
		{"quiet", p.quiet}, {"fungus_hits", p.fung_hit},
		{"flytrap_damage", bytes_to_utf8(p.flytrap_damage)},
		{"was_trapped", p.was_trapped},
		{"old_pos", coord_json(p.old_pos)}, {"old_room", room_ref(g, p.old_room)},
	};
}

json level_json(const Game &g)
{
	const Level &l = g.level;
	json rooms = json::array(), passages = json::array(), monsters = json::array();
	for (const struct room &r : l.rooms)
		rooms.push_back(room_json(r));
	for (const struct room &r : l.passages)
		passages.push_back(room_json(r));
	for (const Creature *tp : l.monsters)
		monsters.push_back(g.pool.creatures.slot_of(tp));
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
		{"bailout", t.bailout}, {"last_count", t.last_count}, {"last_ch", t.last_ch},
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

const json &field(const json &j, const char *key)
{
	if (!j.is_object())
		fail(std::string("expected an object holding \"") + key + "\"");
	auto it = j.find(key);
	if (it == j.end())
		fail(std::string("\"") + key + "\" is missing");
	return *it;
}

// std::in_range() takes no plain char; check it as the char type it is
template <class T>
using RangeOf = std::conditional_t<std::is_same_v<T, char>,
	std::conditional_t<std::is_signed_v<char>, signed char, unsigned char>, T>;

template <class T>
T num(const json &j, const char *key)
{
	const json &v = field(j, key);
	if (!v.is_number_integer())
		fail(std::string("\"") + key + "\" is not a whole number");
	if (v.is_number_unsigned()) {
		auto u = v.get<unsigned long long>();
		if (!std::in_range<RangeOf<T>>(u))
			fail(std::string("\"") + key + "\" is out of range");
		return static_cast<T>(u);
	}
	auto n = v.get<long long>();
	if (!std::in_range<RangeOf<T>>(n))
		fail(std::string("\"") + key + "\" is out of range");
	return static_cast<T>(n);
}

template <class T>
T num_in(const json &j, const char *key, T lo, T hi)
{
	T n = num<T>(j, key);
	if (n < lo || n > hi)
		fail(std::string("\"") + key + "\" is out of range");
	return n;
}

bool flag(const json &j, const char *key)
{
	const json &v = field(j, key);
	if (!v.is_boolean())
		fail(std::string("\"") + key + "\" is not true or false");
	return v.get<bool>();
}

const json &array_of(const json &j, const char *key, std::size_t size)
{
	const json &v = field(j, key);
	if (!v.is_array() || v.size() != size)
		fail(std::string("\"") + key + "\" should be a list of " + std::to_string(size));
	return v;
}

int whole(const json &v, const char *what)
{
	if (!v.is_number_integer())
		fail(std::string(what) + " is not a whole number");
	auto n = v.get<long long>();
	if (!std::in_range<int>(n))
		fail(std::string(what) + " is out of range");
	return static_cast<int>(n);
}

coord to_coord(const json &v, const char *what)
{
	if (!v.is_array() || v.size() != 2)
		fail(std::string(what) + " should be [x, y]");
	return coord{whole(v[0], what), whole(v[1], what)};
}

coord coord_of(const json &j, const char *key)
{
	return to_coord(field(j, key), key);
}

// Text into a char buffer of the given size (with its NUL)
void text_into(char *buf, std::size_t size, const json &j, const char *key)
{
	const json &v = field(j, key);
	if (!v.is_string())
		fail(std::string("\"") + key + "\" is not text");
	std::string bytes = utf8_to_bytes(v.get<std::string>());
	if (bytes.size() >= size)
		fail(std::string("\"") + key + "\" is too long");
	bytes.copy(buf, bytes.size());
	buf[bytes.size()] = '\0';
}

// Null, or text kept for the rest of the program
const char *kept_text(const json &v, const char *what)
{
	if (v.is_null())
		return nullptr;
	if (!v.is_string())
		fail(std::string(what) + " is not text");
	return intern(utf8_to_bytes(v.get<std::string>()));
}

/*
 * A slot number, or nullptr for null. With used, the slot must be in use;
 * without, a free slot gives nullptr (a save made before discard() forgot
 * the last item picked can name the freed slot).
 */
Item *item_at(Game &g, const json &v, const char *what, bool used = true)
{
	if (v.is_null())
		return nullptr;
	int slot = whole(v, what);
	Item *obj = g.pool.items.at(slot);
	if (slot < 0 || slot >= MAXITEMS || (used && obj == nullptr))
		fail(std::string(what) + " is not an item in use");
	return obj;
}

struct room *room_at(Game &g, const json &v, const char *what)
{
	if (v.is_null())
		return nullptr;
	if (v.is_object() && v.size() == 1) {
		if (v.contains("room")) {
			int i = whole(v["room"], what);
			if (i >= 0 && i < MAXROOMS)
				return &g.level.rooms[i];
		} else if (v.contains("passage")) {
			int i = whole(v["passage"], what);
			if (i >= 0 && i < MAXPASS)
				return &g.level.passages[i];
		}
	}
	fail(std::string(what) + " is not a room or passage");
}

coord *dest_at(Game &g, const json &v)
{
	if (v.is_null())
		return nullptr;
	if (v == "hero")
		return &g.player.body.t_pos;
	if (v.is_object() && v.size() == 1) {
		if (v.contains("room_gold")) {
			int i = whole(v["room_gold"], "dest");
			if (i >= 0 && i < MAXROOMS)
				return &g.level.rooms[i].r_gold;
		} else if (v.contains("passage_gold")) {
			int i = whole(v["passage_gold"], "dest");
			if (i >= 0 && i < MAXPASS)
				return &g.level.passages[i].r_gold;
		} else if (v.contains("item")) {
			return &item_at(g, v["item"], "dest")->o_pos;
		}
	}
	fail("\"dest\" is not the hero, gold or an item");
}

const char *damage_at(Game &g, const json &v)
{
	if (v.is_object()) {
		if (v == json{{"alias", "flytrap"}})
			return g.player.flytrap_damage;
		fail("\"damage\" is an unknown alias");
	}
	return kept_text(v, "\"damage\"");
}

struct stats stats_from(Game &g, const json &j)
{
	struct stats s{};
	s.s_str = num<str_t>(j, "str");
	s.s_exp = num<long>(j, "exp");
	s.s_lvl = num<int>(j, "level");
	s.s_arm = num<int>(j, "armor");
	s.s_hpt = num<int>(j, "hp");
	s.s_dmg = damage_at(g, field(j, "damage"));
	s.s_maxhp = num<int>(j, "max_hp");
	return s;
}

void items_into(Game &g, List<Item> &list, const json &slots, const char *what)
{
	if (!slots.is_array())
		fail(std::string(what) + " is not a list");
	// Kept in their order: push each to the back
	const Item *last = nullptr;
	for (const json &v : slots) {
		Item *obj = item_at(g, v, what);
		if (obj == nullptr)
			fail(std::string(what) + " holds a null");
		list.insert_after(last, obj);
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
	c.t_stats = stats_from(g, field(j, "stats"));
	c.t_room = room_at(g, field(j, "room"), "\"room\"");
	items_into(g, c.t_pack, field(j, "pack"), "\"pack\"");
}

void item_from(Item &o, const json &j)
{
	o.o_type = static_cast<ItemKind>(num_in<int>(j, "kind", 0, static_cast<int>(ItemKind::Missile)));
	o.o_pos = coord_of(j, "pos");
	o.o_text = nullptr;
	o.o_launch = num<char>(j, "launch");
	o.o_damage = kept_text(field(j, "damage"), "\"damage\"");
	o.o_hurldmg = kept_text(field(j, "hurl"), "\"hurl\"");
	o.o_count = num<int>(j, "count");
	o.o_which = num<int>(j, "which");
	o.o_hplus = num<int>(j, "hplus");
	o.o_dplus = num<int>(j, "dplus");
	o.o_ac = num<short>(j, "ac");
	o.o_flags = ItemFlags::from_bits(num<ItemFlags::Bits>(j, "flags"));
	o.o_enemy = num<char>(j, "enemy");
	o.o_group = num<int>(j, "group");
}

void room_from(struct room &r, const json &j)
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

// A row of COLS bytes as hex
std::vector<unsigned char> hex_row(const json &v, const char *what)
{
	if (!v.is_string() || v.get_ref<const std::string &>().size() != 2 * COLS)
		fail(std::string(what) + " rows should be " + std::to_string(2 * COLS) + " hex digits");
	const std::string &s = v.get_ref<const std::string &>();
	std::vector<unsigned char> out;
	for (int x = 0; x < COLS; x++) {
		int hi = hex_value(s[2 * x]), lo = hex_value(s[2 * x + 1]);
		if (hi < 0 || lo < 0)
			fail(std::string(what) + " rows should be hex digits");
		out.push_back(static_cast<unsigned char>(hi << 4 | lo));
	}
	return out;
}

void grid_from(unsigned char *grid, const json &j, const char *key)
{
	const json &rows = array_of(j, key, map_rows);
	for (int y = 1; y < maxrow; y++) {
		std::vector<unsigned char> row = hex_row(rows[y - 1], key);
		for (int x = 0; x < COLS; x++)
			grid[INDEX(y, x)] = row[x];
	}
}

void odds_from(struct magic_item *items, int n, const json &j, const char *key)
{
	const json &list = array_of(j, key, n);
	for (int i = 0; i < n; i++) {
		if (!list[i].is_array() || list[i].size() != 2)
			fail(std::string("\"") + key + "\" entries should be [odds, worth]");
		items[i].mi_prob = whole(list[i][0], key);
		int worth = whole(list[i][1], key);
		if (!std::in_range<short>(worth))
			fail(std::string("\"") + key + "\" is out of range");
		items[i].mi_worth = static_cast<short>(worth);
	}
}

template <std::size_t N>
void kept_texts_from(const char *(&texts)[N], const json &j, const char *key)
{
	const json &list = array_of(j, key, N);
	for (std::size_t i = 0; i < N; i++)
		texts[i] = kept_text(list[i], key);
}

template <std::size_t N>
void bools_from(bool (&values)[N], const json &j, const char *key)
{
	const json &list = array_of(j, key, N);
	for (std::size_t i = 0; i < N; i++) {
		if (!list[i].is_boolean())
			fail(std::string("\"") + key + "\" should hold true or false");
		values[i] = list[i].get<bool>();
	}
}

template <std::size_t N>
void guess_refs_from(Items &items, char *(&guesses)[N], const json &j, const char *key)
{
	const json &list = array_of(j, key, N);
	for (std::size_t i = 0; i < N; i++) {
		if (list[i].is_null()) {
			guesses[i] = nullptr;
			continue;
		}
		int n = whole(list[i], key);
		if (n < 0 || n >= static_cast<int>(std::size(items.guesses)))
			fail(std::string("\"") + key + "\" is out of range");
		guesses[i] = items.guesses[n].storage;
	}
}

void items_from(Items &items, const json &j)
{
	const json &odds = field(j, "odds");
	odds_from(items.s_magic, MAXSCROLLS, odds, "scrolls");
	odds_from(items.p_magic, MAXPOTIONS, odds, "potions");
	odds_from(items.r_magic, MAXRINGS, odds, "rings");
	odds_from(items.ws_magic, MAXSTICKS, odds, "sticks");
	odds_from(items.things, NUMTHINGS, odds, "things");

	const json &names = array_of(j, "scroll_names", MAXSCROLLS);
	for (int i = 0; i < MAXSCROLLS; i++)
		text_into(items.s_names[i].storage, sizeof items.s_names[i].storage, json{{"name", names[i]}}, "name");
	kept_texts_from(items.p_colors, j, "potion_colors");
	kept_texts_from(items.r_stones, j, "ring_stones");
	kept_texts_from(items.ws_made, j, "stick_materials");
	kept_texts_from(items.ws_type, j, "stick_types");

	const json &known = field(j, "known");
	bools_from(items.s_know, known, "scrolls");
	bools_from(items.p_know, known, "potions");
	bools_from(items.r_know, known, "rings");
	bools_from(items.ws_know, known, "sticks");

	const json &texts = array_of(j, "guesses", std::size(items.guesses));
	for (std::size_t i = 0; i < std::size(items.guesses); i++)
		text_into(items.guesses[i].storage, sizeof items.guesses[i].storage, json{{"guess", texts[i]}}, "guess");
	const json &guessed = field(j, "guessed");
	guess_refs_from(items, items.s_guess, guessed, "scrolls");
	guess_refs_from(items, items.p_guess, guessed, "potions");
	guess_refs_from(items, items.r_guess, guessed, "rings");
	guess_refs_from(items, items.ws_guess, guessed, "sticks");
	items.iguess = num_in<int>(j, "next_guess", 0, std::size(items.guesses));
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
	const json &rooms = array_of(j, "rooms", MAXROOMS);
	for (int i = 0; i < MAXROOMS; i++)
		room_from(l.rooms[i], rooms[i]);
	const json &passages = array_of(j, "passages", MAXPASS);
	for (int i = 0; i < MAXPASS; i++)
		room_from(l.passages[i], passages[i]);
	grid_from(l.map, j, "map");
	grid_from(l.flags, j, "flags");
	items_into(g, l.objects, field(j, "objects"), "\"objects\"");

	const json &monsters = field(j, "monsters");
	if (!monsters.is_array())
		fail("\"monsters\" is not a list");
	const Creature *last = nullptr;
	for (const json &v : monsters) {
		int slot = whole(v, "\"monsters\"");
		Creature *tp = g.pool.creatures.at(slot);
		if (tp == nullptr)
			fail("\"monsters\" holds a creature not in use");
		l.monsters.insert_after(last, tp);
		last = tp;
	}
}

void player_from(Game &g, const json &j)
{
	Player &p = g.player;
	creature_from(g, p.body, field(j, "body"));
	p.max_stats = stats_from(g, field(j, "max_stats"));
	p.purse = num<int>(j, "purse");
	p.in_pack = num<int>(j, "in_pack");
	p.armor = item_at(g, field(j, "armor"), "\"armor\"");
	p.weapon = item_at(g, field(j, "weapon"), "\"weapon\"");
	const json &rings = array_of(j, "rings", 2);
	p.rings[0] = item_at(g, rings[0], "\"rings\"");
	p.rings[1] = item_at(g, rings[1], "\"rings\"");
	p.food_left = num<int>(j, "food_left");
	p.hungry_state = num<int>(j, "hungry_state");
	p.has_amulet = flag(j, "has_amulet");
	p.saw_amulet = flag(j, "saw_amulet");
	p.max_level = num<int>(j, "max_level");
	p.no_command = num<int>(j, "no_command");
	p.no_move = num<int>(j, "no_move");
	p.quiet = num<int>(j, "quiet");
	p.fung_hit = num<int>(j, "fungus_hits");
	text_into(p.flytrap_damage, sizeof p.flytrap_damage, j, "flytrap_damage");
	p.was_trapped = num<unsigned char>(j, "was_trapped");
	p.old_pos = coord_of(j, "old_pos");
	p.old_room = room_at(g, field(j, "old_room"), "\"old_room\"");
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
	t.typeahead = kept_text(field(j, "typeahead"), "\"typeahead\"");
	if (t.typeahead == nullptr)
		fail("\"typeahead\" is null");
	t.bailout = flag(j, "bailout");
	t.last_count = num<int>(j, "last_count");
	t.last_ch = num<unsigned char>(j, "last_ch");
	t.last_take = num<unsigned char>(j, "last_take");
	t.do_take = num<unsigned char>(j, "do_take");
	t.last_item_key = num<unsigned char>(j, "last_item_key");
	t.last_item = item_at(g, field(j, "last_item"), "\"last_item\"", false);
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
	text_into(g.options.name, sizeof g.options.name, options, "name");
	text_into(g.options.fruit, sizeof g.options.fruit, options, "fruit");
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
	text_into(g.message.text, sizeof g.message.text, message, "text");
	text_into(g.message.last, sizeof g.message.last, message, "last");
	g.message.end = num_in<int>(message, "end", 0, COLS);
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
write_save(const char *path, const Game &g, const MapView &view)
{
	std::string temp = std::string(path) + ".tmp";
	{
		std::ofstream file(temp, std::ios::binary | std::ios::trunc);
		file << format_save(g, view);
		file.close();
		if (!file) {
			std::remove(temp.c_str());
			return std::unexpected(SaveError{SaveError::Kind::Unreadable, std::string("can't write ") + temp});
		}
	}
	if (std::rename(temp.c_str(), path) != 0) {
		std::remove(temp.c_str());
		return std::unexpected(SaveError{SaveError::Kind::Unreadable, std::string("can't write ") + path});
	}
	return {};
}

std::expected<void, SaveError>
read_save(const char *path, Game &g, MapView &view)
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
