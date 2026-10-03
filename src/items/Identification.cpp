#include "items/Identification.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "core/Ascii.hpp"
#include "core/Config.hpp"
#include "core/Coord.hpp"
#include "core/Glyphs.hpp"
#include "core/KindTable.hpp"
#include "core/Maybe.hpp"
#include "core/Text.hpp"
#include "entities/Item.hpp"
#include "entities/MonsterCatalog.hpp"
#include "game/Game.hpp"
#include "game/Keyboard.hpp"
#include "game/Messages.hpp"
#include "items/Inventory.hpp"
#include "items/ItemCatalog.hpp"
#include "items/Kinds.hpp"
#include "items/effects/Ring.hpp"
#include "items/effects/Wand.hpp"
#include "items/effects/Weapon.hpp"
#include "ui/Display.hpp"
#include "ui/Input.hpp"

namespace rogue::items {

namespace {

void	print_disc(ItemKind type);
void	set_order(std::span<short> order);
std::string	nothing(ItemKind type);

}  // namespace

/*
 * inv_name:
 *	Return the name of something as it would appear in an
 *	inventory.
 */
std::string
inv_name(const Item &obj, bool drop)
{
	std::string name;
	rogue::Items &items = game().items;
	bool brief = game().options.brief();

	switch (obj.o_type)
	{
	case ItemKind::Scroll: {
		Scroll which = obj.which<Scroll>();

		if (obj.o_count == 1)
			name = "A scroll ";
		else
			name = std::format("{} scrolls ", obj.o_count);
		if (items.s_know[which])
			name += std::format("of {}", items.s_magic[which].mi_name);
		else if (!items.s_guess[which].empty())
			name += std::format("called {}", items.s_guess[which]);
		else if (brief)
			name += std::format("titled '{:.17}'", items.s_names[which]);
		else
			name += std::format("titled '{}'", items.s_names[which]);
		break;
	}
	case ItemKind::Potion: {
		Potion which = obj.which<Potion>();

		if (obj.o_count == 1)
			name = "A potion ";
		else
			name = std::format("{} potions ", obj.o_count);
		if (items.p_know[which])
			name += brief ? std::format("of {}", items.p_magic[which].mi_name)
				: std::format("of {}({})", items.p_magic[which].mi_name, items.p_colors[which]);
		else if (!items.p_guess[which].empty())
			name += brief ? std::format("called {}", items.p_guess[which])
				: std::format("called {}({})", items.p_guess[which], items.p_colors[which]);
		else if (obj.o_count == 1)
			name = std::format("A{} {} potion", vowelstr(items.p_colors[which]),
				items.p_colors[which]);
		else
			name = std::format("{} {} potions", obj.o_count, items.p_colors[which]);
		break;
	}
	case ItemKind::Food: {
		Food which = obj.which<Food>();

		if (which == Food::Fruit)
			if (obj.o_count == 1)
				name = std::format("A{} {}", vowelstr(game().options.fruit),
					game().options.fruit);
			else
				name = std::format("{} {}s", obj.o_count,
					game().options.fruit);
		else
			if (obj.o_count == 1)
				name = "Some food";
			else
				name = std::format("{} rations of food", obj.o_count);
		break;
	}
	case ItemKind::Weapon: {
		WeaponType which = obj.which<WeaponType>();

		if (obj.o_count > 1)
			name = std::format("{} ", obj.o_count);
		else
			name = std::format("A{} ", vowelstr(w_names[which]));
		if (obj.o_flags.test(ItemFlag::Known))
			name += std::format("{} {}", items::effects::num(obj.o_hplus, obj.o_dplus, WEAPON),
				w_names[which]);
		else
			name += w_names[which];
		if (obj.o_count > 1)
			name += "s";
		if (obj.o_enemy && obj.o_flags.test(ItemFlag::Revealed))
			name += std::format(" of {} slaying", entities::monsters[obj.o_enemy-'A'].m_name);
		break;
	}
	case ItemKind::Armor: {
		ArmorType which = obj.which<ArmorType>();

		if (!obj.o_flags.test(ItemFlag::Known))
			name = a_names[which];
		else if (brief)
			name = std::format("{} {}", items::effects::num(a_class[which] - obj.o_ac, 0, ARMOR),
				a_names[which]);
		else
			name = std::format("{} {} [armor class {}]", items::effects::num(a_class[which] - obj.o_ac, 0, ARMOR),
				a_names[which], -(obj.o_ac-11));
		break;
	}
	case ItemKind::Amulet:
		name = "The Amulet of Yendor";
		break;
	case ItemKind::Stick: {
		Stick which = obj.which<Stick>();

		name = std::format("A{} {} ", vowelstr(items.ws_type[which]), items.ws_type[which]);
		if (items.ws_know[which])
			name += brief ? std::format("of {}{}", items.ws_magic[which].mi_name, items::effects::charge_str(obj))
				: std::format("of {}{}({})", items.ws_magic[which].mi_name,
					items::effects::charge_str(obj), items.ws_made[which]);
		else if (!items.ws_guess[which].empty())
			name += brief ? std::format("called {}", items.ws_guess[which])
				: std::format("called {}({})", items.ws_guess[which], items.ws_made[which]);
		else {
			/*
			 * The original wrote this over the name from its third
			 * character, keeping "A " even before a vowel ("A oak staff").
			 */
			name.resize(2);
			name += std::format("{} {}", items.ws_made[which], items.ws_type[which]);
		}
		break;
	}
	case ItemKind::Ring: {
		Ring which = obj.which<Ring>();

		if (items.r_know[which])
			name = brief ? std::format("A{} ring of {}", items::effects::ring_num(obj), items.r_magic[which].mi_name)
				: std::format("A{} ring of {}({})", items::effects::ring_num(obj),
					items.r_magic[which].mi_name, items.r_stones[which]);
		else if (!items.r_guess[which].empty())
			name = brief ? std::format("A ring called {}", items.r_guess[which])
				: std::format("A ring called {}({})", items.r_guess[which], items.r_stones[which]);
		else
			name = std::format("A{} {} ring", vowelstr(items.r_stones[which]),
				items.r_stones[which]);
		break;
	}
	default:	// the other kinds of item: nothing, except to the checks
		if constexpr (rogue::config::debug_checks) {
			if (obj.o_type == ItemKind::Gold)
				name = std::format("Gold at {},{}", obj.o_pos.y, obj.o_pos.x);
			else {
				debug("Picked up someting bizzare {}", io_unctrl(glyph_of(obj.o_type)));
				name = std::format("Something bizarre {}({})", static_cast<char>(glyph_of(obj.o_type)),
					static_cast<int>(obj.o_type));
			}
		}
		break;
	}
	if (refers_to(game().player.armor_item(), obj))
		name += " (being worn)";
	if (refers_to(game().player.weapon_item(), obj))
		name += " (weapon in hand)";
	if (refers_to(game().player.ring_item(Hand::Left), obj))
		name += " (on left hand)";
	else if (refers_to(game().player.ring_item(Hand::Right), obj))
		name += " (on right hand)";
	if (!name.empty()) {
		if (drop && is_monster(name[0]))
			name[0] = to_lower(name[0]);
		else if (!drop && is_lower(name[0]))
			name[0] = to_upper(name[0]);
	}
	return name;
}

namespace {

/*
 * discovered:
 *	list what the player has discovered in this game of a certain type
 */
int line_cnt = 0;

}  // namespace

void
discovered()
{
	print_disc(ItemKind::Potion);
	add_line("", " ");
	print_disc(ItemKind::Scroll);
	add_line("", " ");
	print_disc(ItemKind::Ring);
	add_line("", " ");
	print_disc(ItemKind::Stick);
	end_line("");
}

/*
 * print_disc:
 *	Print what we've discovered of type 'type'
 */

namespace {

void
print_disc(ItemKind type)
{
	std::span<const bool> know;
	std::span<const std::string> guess;
	int i, maxnum = 0, num_found;
	Item obj{};
	std::array<short, std::max({kind_count<Scroll>, kind_count<Potion>, kind_count<Ring>, kind_count<Stick>})> order;
	rogue::Items &items = game().items;

	switch (type)
	{
	case ItemKind::Scroll:
		maxnum = kind_count<Scroll>;
		know = items.s_know;
		guess = items.s_guess;
		break;
	case ItemKind::Potion:
		maxnum = kind_count<Potion>;
		know = items.p_know;
		guess = items.p_guess;
		break;
	case ItemKind::Ring:
		maxnum = kind_count<Ring>;
		know = items.r_know;
		guess = items.r_guess;
		break;
	case ItemKind::Stick:
		maxnum = kind_count<Stick>;
		know = items.ws_know;
		guess = items.ws_guess;
		break;
	default:	// the other kinds of item: nothing
		break;
	}
	set_order(std::span(order).first(maxnum));
	obj.o_count = 1;
	obj.o_flags.reset();
	num_found = 0;
	for (i = 0; i < maxnum; i++)
		if (know[order[i]] || !guess[order[i]].empty())
		{
			obj.o_type = type;
			obj.o_which = order[i];
			add_line("", inv_name(obj, false));
			num_found++;
		}
	if (num_found == 0)
		add_line("", nothing(type));
}

/*
 * set_order:
 *	Set up order for list
 */
void
set_order(std::span<short> order)
{
	int i, r, t;
	int numthings = static_cast<int>(order.size());

	for (i = 0; i< numthings; i++)
		order[i] = i;

	for (i = numthings; i > 0; i--)
	{
		r = rnd(i);
		t = order[i - 1];
		order[i - 1] = order[r];
		order[r] = t;
	}
}

}  // namespace

/*
 * add_line:
 *	Add a line to the list of discoveries; null ends the page
 *	(end_line())
 */
unsigned char
add_line(std::string_view use, std::optional<std::string_view> line)
{
	unsigned char retchar = ' ';
	if (line_cnt == 0)
	{
		ui::display().open_page();
		ui::display().clear_page();
	}
	if (line_cnt >= MAXLINES - 1 || !line)
	{
		if (!use.empty())
			ui::display().write_at(MAXLINES-1, 0,
				std::format("-Select item to {}. Esc to cancel-", use));
		else
			ui::display().write_at(MAXLINES-1, 0, "-Press space to continue-");
		do
			retchar = readchar();
		while (retchar != ESCAPE && retchar != ' ' && (!is_lower(retchar)));
		ui::display().clear_page();
		line_cnt = 0;
	}
	if (line && !(line_cnt == 0 && line->empty()))
	{
		Coord end;

		end = ui::display().write_at(line_cnt, 0, *line);
		/*
		 * if the line wrapped but nothing was printed on this
		 * line you might as well use it for the next item
		 */
		if (end.x != 0)
			line_cnt = end.y + 1;
	}
	return(retchar);
}

/*
 * end_line:
 *	End the list of lines
 */
unsigned char
end_line(std::string_view use)
{
	int retchar;

	retchar = add_line(use, std::nullopt);
	ui::display().close_page();
	line_cnt = 0;
	return(retchar);
}

namespace {

/*
 * nothing:
 *	The message for "nothing found"
 */
std::string
nothing(ItemKind type)
{
	std::string_view tystr;

	switch (type)
	{
		case ItemKind::Potion: tystr = "potion"; break;
		case ItemKind::Scroll: tystr = "scroll"; break;
		case ItemKind::Ring: tystr = "ring"; break;
		case ItemKind::Stick: tystr = "stick"; break;
		// avoid possibly uninitialized use of tystr
		default: tystr = "item";
	}
	return std::format("{} about any {}s",
		game().options.terse ? "Nothing" : "Haven't discovered anything", tystr);
}

}  // namespace

/*
 * call_it:
 *	Call an object something after use.
 */
void
call_it(bool know, std::string &guess)
{
	if (know && !guess.empty())
		guess.clear();
	else if (!know && guess.empty()) {
		msg("{}call it? ",noterse("what do you want to "));
		if (auto name = ui::input().read_line(MAXNAME))
			guess = *name;
		msg("");
	}
}

/*
 * call:
 *	Allow a user to call a potion, scroll, or ring something
 */
void
call()
{
	Maybe<Item> obj;
	std::span<std::string> guess;
	std::string_view elsewise;
	std::span<const bool> know;
	rogue::Items &items = game().items;

	obj = get_item("call", ItemFilter::callable());
	/*
	 * Make certain that it is somethings that we want to wear
	 */
	if (!obj)
		return;
	switch (obj->o_type)
	{
	case ItemKind::Ring:
		guess = items.r_guess;
		know = items.r_know;
		elsewise = (!guess[obj->o_which].empty() ?
			guess[obj->o_which] : items.r_stones[obj->which<Ring>()]);
		break;
	case ItemKind::Potion:
		guess = items.p_guess;
		know = items.p_know;
		elsewise = (!guess[obj->o_which].empty() ?
			guess[obj->o_which] : items.p_colors[obj->which<Potion>()]);
		break;
	case ItemKind::Scroll:
		guess = items.s_guess;
		know = items.s_know;
		elsewise = (!guess[obj->o_which].empty() ?
			guess[obj->o_which] : items.s_names[obj->which<Scroll>()]);
		break;
	case ItemKind::Stick:
		guess = items.ws_guess;
		know = items.ws_know;
		elsewise = (!guess[obj->o_which].empty() ?
			guess[obj->o_which] : items.ws_made[obj->which<Stick>()]);
		break;
	default:
		msg("you can't call that anything");
		return;
	}
	if (know[obj->o_which])
	{
		msg("that has already been identified");
		return;
	}
	msg("Was called \"{}\"", elsewise);
	msg("what do you want to call it? ");
	if (auto name = ui::input().read_line(MAXNAME); name && !name->empty())
		guess[obj->o_which] = *name;
	msg("");
}

/*
 * whatis comes from wizard.c (wizard.c	1.4 (AI Design)	12/14/84).
 */

/*
 * whatis:
 *	What a certain object is: identify a kind (a scroll of identify)
 */
void
whatis()
{
	Maybe<Item> obj;
	rogue::Items &items = game().items;

	if (game().player.body.t_pack.empty()) {
		msg("You don't have anything in your pack to identify");
		return;
	}

	for (;;) {
		if (!(obj = get_item("identify", ItemFilter::all()))) {
			msg("You must identify something");
			msg(" ");
			game().message.end = 0;
		} else
			break;
	}

	switch (obj->o_type) {
	case ItemKind::Scroll:
		items.s_know[obj->which<Scroll>()] = true;
		items.s_guess[obj->which<Scroll>()].clear();
		break;
	case ItemKind::Potion:
		items.p_know[obj->which<Potion>()] = true;
		items.p_guess[obj->which<Potion>()].clear();
		break;
	case ItemKind::Stick:
		items.ws_know[obj->which<Stick>()] = true;
		obj->o_flags.set(ItemFlag::Known);
		items.ws_guess[obj->which<Stick>()].clear();
		break;
	case ItemKind::Weapon:
	case ItemKind::Armor:
		obj->o_flags.set(ItemFlag::Known);
		break;
	case ItemKind::Ring:
		items.r_know[obj->which<Ring>()] = true;
		obj->o_flags.set(ItemFlag::Known);
		items.r_guess[obj->which<Ring>()].clear();
		break;
	default:	// the other kinds of item: nothing
		break;
	}
	/*
	 * If it is vorpally enchanted, then reveal what type of monster it is
	 * vorpally enchanted against
	 */
	if (obj->o_enemy)
		obj->o_flags.set(ItemFlag::Revealed);
	msg("{}", inv_name(*obj, false));
}

}  // namespace rogue::items
