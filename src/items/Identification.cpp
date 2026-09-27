#include "rogue.h"

namespace rogue::items {

static void	print_disc(ItemKind type);
static void	set_order(short *order, int numthings);
static std::string	nothing(ItemKind type);

/*
 * inv_name:
 *	Return the name of something as it would appear in an
 *	inventory.
 */
std::string
inv_name(const Item *obj, bool drop)
{
	std::string name;
	rogue::Items &items = game().items;
	bool brief = game().options.brief();

	switch (obj->o_type)
	{
	case ItemKind::Scroll: {
		Scroll which = obj->which<Scroll>();

		if (obj->o_count == 1)
			name = "A scroll ";
		else
			name = std::format("{} scrolls ", obj->o_count);
		if (items.s_know[which])
			name += std::format("of {}", items.s_magic[which].mi_name);
		else if (*items.s_guess[which])
			name += std::format("called {}", items.s_guess[which]);
		else if (brief)
			name += std::format("titled '{:.17}'", static_cast<const char *>(items.s_names[which].storage));
		else
			name += std::format("titled '{}'", static_cast<const char *>(items.s_names[which].storage));
		break;
	}
	case ItemKind::Potion: {
		Potion which = obj->which<Potion>();

		if (obj->o_count == 1)
			name = "A potion ";
		else
			name = std::format("{} potions ", obj->o_count);
		if (items.p_know[which])
			name += brief ? std::format("of {}", items.p_magic[which].mi_name)
				: std::format("of {}({})", items.p_magic[which].mi_name, items.p_colors[which]);
		else if (*items.p_guess[which])
			name += brief ? std::format("called {}", items.p_guess[which])
				: std::format("called {}({})", items.p_guess[which], items.p_colors[which]);
		else if (obj->o_count == 1)
			name = std::format("A{} {} potion", vowelstr(items.p_colors[which]),
				items.p_colors[which]);
		else
			name = std::format("{} {} potions", obj->o_count, items.p_colors[which]);
		break;
	}
	case ItemKind::Food: {
		Food which = obj->which<Food>();

		if (which == Food::Fruit)
			if (obj->o_count == 1)
				name = std::format("A{} {}", vowelstr(game().options.fruit),
					static_cast<const char *>(game().options.fruit));
			else
				name = std::format("{} {}s", obj->o_count,
					static_cast<const char *>(game().options.fruit));
		else
			if (obj->o_count == 1)
				name = "Some food";
			else
				name = std::format("{} rations of food", obj->o_count);
		break;
	}
	case ItemKind::Weapon: {
		WeaponType which = obj->which<WeaponType>();

		if (obj->o_count > 1)
			name = std::format("{} ", obj->o_count);
		else
			name = std::format("A{} ", vowelstr(w_names[which]));
		if (obj->o_flags.test(ISKNOW))
			name += std::format("{} {}", num(obj->o_hplus, obj->o_dplus, WEAPON),
				w_names[which]);
		else
			name += w_names[which];
		if (obj->o_count > 1)
			name += "s";
		if (obj->o_enemy && obj->o_flags.test(ISREVEAL))
			name += std::format(" of {} slaying", monsters[obj->o_enemy-'A'].m_name);
		break;
	}
	case ItemKind::Armor: {
		ArmorType which = obj->which<ArmorType>();

		if (!obj->o_flags.test(ISKNOW))
			name = a_names[which];
		else if (brief)
			name = std::format("{} {}", num(a_class[which] - obj->o_ac, 0, ARMOR),
				a_names[which]);
		else
			name = std::format("{} {} [armor class {}]", num(a_class[which] - obj->o_ac, 0, ARMOR),
				a_names[which], -(obj->o_ac-11));
		break;
	}
	case ItemKind::Amulet:
		name = "The Amulet of Yendor";
		break;
	case ItemKind::Stick: {
		Stick which = obj->which<Stick>();

		name = std::format("A{} {} ", vowelstr(items.ws_type[which]), items.ws_type[which]);
		if (items.ws_know[which])
			name += brief ? std::format("of {}{}", items.ws_magic[which].mi_name, charge_str(obj))
				: std::format("of {}{}({})", items.ws_magic[which].mi_name,
					charge_str(obj), items.ws_made[which]);
		else if (*items.ws_guess[which])
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
		Ring which = obj->which<Ring>();

		if (items.r_know[which])
			name = brief ? std::format("A{} ring of {}", ring_num(obj), items.r_magic[which].mi_name)
				: std::format("A{} ring of {}({})", ring_num(obj),
					items.r_magic[which].mi_name, items.r_stones[which]);
		else if (*items.r_guess[which])
			name = brief ? std::format("A ring called {}", items.r_guess[which])
				: std::format("A ring called {}({})", items.r_guess[which], items.r_stones[which]);
		else
			name = std::format("A{} {} ring", vowelstr(items.r_stones[which]),
				items.r_stones[which]);
		break;
	}
	default:	// the other kinds of item: nothing, except to the checks
		if constexpr (rogue::config::debug_checks) {
			if (obj->o_type == ItemKind::Gold)
				name = std::format("Gold at {},{}", obj->o_pos.y, obj->o_pos.x);
			else {
				debug("Picked up someting bizzare {}", io_unctrl(glyph_of(obj->o_type)));
				name = std::format("Something bizarre {}({})", static_cast<char>(glyph_of(obj->o_type)),
					static_cast<int>(obj->o_type));
			}
		}
		break;
	}
	if (obj == game().player.armor)
		name += " (being worn)";
	if (obj == game().player.weapon)
		name += " (weapon in hand)";
	if (obj == game().player.rings[Hand::Left])
		name += " (on left hand)";
	else if (obj == game().player.rings[Hand::Right])
		name += " (on right hand)";
	if (!name.empty()) {
		if (drop && is_monster(name[0]))
			name[0] = tolower(name[0]);
		else if (!drop && is_lower(name[0]))
			name[0] = toupper(name[0]);
	}
	return name;
}

/*
 * discovered:
 *	list what the player has discovered in this game of a certain type
 */
static int line_cnt = 0;

void
discovered(void)
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

static
void
print_disc(ItemKind type)
{
	bool *know = nullptr;
	char **guess = nullptr;
	int i, maxnum = 0, num_found;
	static Item obj;
	static short order[std::max({kind_count<Scroll>, kind_count<Potion>, kind_count<Ring>, kind_count<Stick>})];
	rogue::Items &items = game().items;

	switch (type)
	{
	case ItemKind::Scroll:
		maxnum = kind_count<Scroll>;
		know = items.s_know.data();
		guess = items.s_guess.data();
		break;
	case ItemKind::Potion:
		maxnum = kind_count<Potion>;
		know = items.p_know.data();
		guess = items.p_guess.data();
		break;
	case ItemKind::Ring:
		maxnum = kind_count<Ring>;
		know = items.r_know.data();
		guess = items.r_guess.data();
		break;
	case ItemKind::Stick:
		maxnum = kind_count<Stick>;
		know = items.ws_know.data();
		guess = items.ws_guess.data();
		break;
	default:	// the other kinds of item: nothing
		break;
	}
	set_order(order, maxnum);
	obj.o_count = 1;
	obj.o_flags.reset();
	num_found = 0;
	for (i = 0; i < maxnum; i++)
		if (know[order[i]] || *guess[order[i]])
		{
			obj.o_type = type;
			obj.o_which = order[i];
			add_line("", inv_name(&obj, false).c_str());
			num_found++;
		}
	if (num_found == 0)
		add_line("", nothing(type).c_str());
}

/*
 * set_order:
 *	Set up order for list
 */
static
void
set_order(short *order, int numthings)
{
	int i, r, t;

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

/*
 * add_line:
 *	Add a line to the list of discoveries; null ends the page
 *	(end_line())
 */
unsigned char
add_line(const char *use, const char *line)
{
	unsigned char retchar = ' ';
	if (line_cnt == 0)
	{
		display().open_page();
		display().clear_page();
	}
	if (line_cnt >= LINES - 1 || line == nullptr)
	{
		if (*use)
			display().write_at(LINES-1, 0,
				std::format("-Select item to {}. Esc to cancel-", use));
		else
			display().write_at(LINES-1, 0, "-Press space to continue-");
		do
			retchar = readchar();
		while (retchar != ESCAPE && retchar != ' ' && (!is_lower(retchar)));
		display().clear_page();
		line_cnt = 0;
	}
	if (line != nullptr && !(line_cnt == 0 && *line == '\0'))
	{
		coord end;

		end = display().write_at(line_cnt, 0, line);
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
end_line(const char *use)
{
	int retchar;

	retchar = add_line(use, nullptr);
	display().close_page();
	line_cnt = 0;
	return(retchar);
}

/*
 * nothing:
 *	The message for "nothing found"
 */
static
std::string
nothing(ItemKind type)
{
	const char *tystr;

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

}  // namespace rogue::items
