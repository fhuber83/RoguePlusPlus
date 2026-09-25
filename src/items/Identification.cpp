#include "rogue.h"

namespace rogue::items {

static void	chopmsg(char *s, const char *shmsg, const char *lnmsg, ...);
static void	print_disc(ItemKind type);
static void	set_order(short *order, int numthings);
static char	*nothing(ItemKind type);

/*
 * inv_name:
 *	Return the name of something as it would appear in an
 *	inventory.
 */
char *
inv_name(Item *obj, bool drop)
{
	int which = obj->o_which;
	char *pb;
	rogue::Items &items = game().items;

	pb = prbuf;
	switch (obj->o_type)
	{
	case ItemKind::Scroll:
		if (obj->o_count == 1) {
			strcpy(pb, "A scroll ");
			pb = &prbuf[9];
		} else {
			sprintf(pb, "%d scrolls ", obj->o_count);
			pb = &prbuf[strlen(prbuf)];
		}
		if (items.s_know[which])
			sprintf(pb, "of %s", items.s_magic[which].mi_name);
		else if (*items.s_guess[which])
			sprintf(pb, "called %s", items.s_guess[which]);
		else
			chopmsg(pb, "titled '%.17s'","titled '%s'", &items.s_names[which]);
		break;
	case ItemKind::Potion:
		if (obj->o_count == 1)
		{
			strcpy(pb, "A potion ");
			pb = &prbuf[9];
		}
		else
		{
			sprintf(pb, "%d potions ", obj->o_count);
			pb = &pb[strlen(prbuf)];
		}
		if (items.p_know[which]) {
			chopmsg(pb, "of %s", "of %s(%s)",
				items.p_magic[which].mi_name, items.p_colors[which]);
		}
		else if (*items.p_guess[which]) {
			chopmsg(pb, "called %s","called %s(%s)", items.p_guess[which],
				items.p_colors[which]);
		}
		else if (obj->o_count == 1)
			sprintf(prbuf, "A%s %s potion", vowelstr(items.p_colors[which]),
				items.p_colors[which]);
		else
			sprintf(prbuf, "%d %s potions", obj->o_count, items.p_colors[which]);
		break;
	case ItemKind::Food:
		if (which == 1)
			if (obj->o_count == 1)
				sprintf(pb, "A%s %s", vowelstr(game().options.fruit), game().options.fruit);
			else
				sprintf(pb, "%d %ss", obj->o_count, game().options.fruit);
		else
			if (obj->o_count == 1)
				strcpy(pb, "Some food");
			else
				sprintf(pb, "%d rations of food", obj->o_count);
		break;
	case ItemKind::Weapon:
		if (obj->o_count > 1)
			sprintf(pb, "%d ", obj->o_count);
		else
			sprintf(pb, "A%s ", vowelstr(w_names[which]));
		pb = &prbuf[strlen(prbuf)];
		if (obj->o_flags.test(ISKNOW))
			sprintf(pb, "%s %s", num(obj->o_hplus, obj->o_dplus, WEAPON),
				w_names[which]);
		else
			sprintf(pb, "%s", w_names[which]);
		if (obj->o_count > 1)
			strcat(pb, "s");
		if (obj->o_enemy && obj->o_flags.test(ISREVEAL))
		{
			strcat(pb, " of ");
			strcat(pb, monsters[obj->o_enemy-'A'].m_name);
			strcat(pb, " slaying");
		}
		break;
	case ItemKind::Armor:
		if (obj->o_flags.test(ISKNOW))
			chopmsg(pb, "%s %s","%s %s [armor class %d]",
				num(a_class[which] - obj->o_ac, 0, ARMOR),
				a_names[which], -(obj->o_ac-11));
		else
			sprintf(pb, "%s", a_names[which]);
		break;
	case ItemKind::Amulet:
		strcpy(pb, "The Amulet of Yendor");
		break;
	case ItemKind::Stick:
		sprintf(pb, "A%s %s ", vowelstr(items.ws_type[which]),
		items.ws_type[which]);
		pb = &prbuf[strlen(prbuf)];
		if (items.ws_know[which])
			chopmsg(pb, "of %s%s", "of %s%s(%s)",
				items.ws_magic[which].mi_name,
				charge_str(obj), items.ws_made[which]);
		else if (*items.ws_guess[which])
			chopmsg(pb, "called %s", "called %s(%s)", items.ws_guess[which],
				items.ws_made[which]);
		else
			sprintf(pb = &prbuf[2], "%s %s", items.ws_made[which], items.ws_type[which]);
		break;
	case ItemKind::Ring:
		if (items.r_know[which])
			chopmsg(pb, "A%s ring of %s", "A%s ring of %s(%s)", ring_num(obj),
				items.r_magic[which].mi_name, items.r_stones[which]);
		else if (*items.r_guess[which])
			chopmsg(pb, "A ring called %s", "A ring called %s(%s)",
				items.r_guess[which], items.r_stones[which]);
		else
			sprintf(pb, "A%s %s ring", vowelstr(items.r_stones[which]),
				items.r_stones[which]);
		break;
#ifdef DEBUG
	case ItemKind::Gold:
		sprintf(pb, "Gold at %d,%d", obj->o_pos.y, obj->o_pos.x);
		break;
	default:
		debug("Picked up someting bizzare %s", io_unctrl(glyph_of(obj->o_type)));
		sprintf(pb, "Something bizarre %c(%d)", glyph_of(obj->o_type),
			static_cast<int>(obj->o_type));
#else
	default:	//@ the other kinds of item: nothing
#endif
		break;
	}
	if (obj == game().player.armor)
		strcat(pb, " (being worn)");
	if (obj == game().player.weapon)
		strcat(pb, " (weapon in hand)");
	if (obj == game().player.rings[LEFT])
		strcat(pb, " (on left hand)");
	else if (obj == game().player.rings[RIGHT])
		strcat(pb, " (on right hand)");
	if (drop && ismonster(prbuf[0]))
		prbuf[0] = tolower(prbuf[0]);
	else if (!drop && is_lower(*prbuf))
		*prbuf = toupper(*prbuf);
	return prbuf;
}

//@ changed original signature to use varargs
static
void
chopmsg(char *s, const char *shmsg, const char *lnmsg, ...)
{
	va_list argp;
	va_start(argp, lnmsg);
	vsnprintf(s, MAXSTR, game().options.brief() ? shmsg : lnmsg, argp);
	va_end(argp);
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
	add_line(nullstr, " ", "");
	print_disc(ItemKind::Scroll);
	add_line(nullstr, " ", "");
	print_disc(ItemKind::Ring);
	add_line(nullstr, " ", "");
	print_disc(ItemKind::Stick);
	end_line(nullstr);
}

/*
 * print_disc:
 *	Print what we've discovered of type 'type'
 */

#define MAX(a,b,c,d) (a>b?(a>c?(a>d?a:d):(c>d?c:d)):(b>c?(b>d?b:d):(c>d?c:d)))

static
void
print_disc(ItemKind type)
{
	bool *know = NULL;
	char **guess = NULL;
	int i, maxnum = 0, num_found;
	static Item obj;
	static short order[MAX(MAXSCROLLS, MAXPOTIONS, MAXRINGS, MAXSTICKS)];
	rogue::Items &items = game().items;

	switch (type)
	{
	case ItemKind::Scroll:
		maxnum = MAXSCROLLS;
		know = items.s_know;
		guess = items.s_guess;
		break;
	case ItemKind::Potion:
		maxnum = MAXPOTIONS;
		know = items.p_know;
		guess = items.p_guess;
		break;
	case ItemKind::Ring:
		maxnum = MAXRINGS;
		know = items.r_know;
		guess = items.r_guess;
		break;
	case ItemKind::Stick:
		maxnum = MAXSTICKS;
		know = items.ws_know;
		guess = items.ws_guess;
		break;
	default:	//@ the other kinds of item: nothing
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
			add_line(nullstr, "%s", inv_name(&obj, FALSE));
			num_found++;
		}
	if (num_found == 0)
		add_line(nullstr, nothing(type), "");
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
 *	Add a line to the list of discoveries
 *
 * VARARGS1
 */
unsigned char
add_line(const char *use, const char *fmt, const char *arg)
{
	char buf[132];  //@ as printw() had
	unsigned char retchar = ' ';
	if (line_cnt == 0)
	{
		display().open_page();
		display().clear_page();
	}
	if (line_cnt >= LINES - 1 || fmt == NULL)
	{
		if (*use)
		{
			snprintf(buf, sizeof buf, "-Select item to %s. Esc to cancel-", use);
			display().write_at(LINES-1, 0, buf);
		}
		else
			display().write_at(LINES-1, 0, "-Press space to continue-");
		do
			retchar = readchar();
		while (retchar != ESCAPE && retchar != ' ' && (!is_lower(retchar)));
		display().clear_page();
		line_cnt = 0;
	}
	if (fmt != NULL && !(line_cnt == 0 && *fmt == '\0'))
	{
		coord end;

		snprintf(buf, sizeof buf, fmt, arg);
		end = display().write_at(line_cnt, 0, buf);
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

	retchar = add_line(use, NULL, "");
	display().close_page();
	line_cnt = 0;
	return(retchar);
}

/*
 * nothing:
 *	Set up prbuf so that message for "nothing found" is there
 */
static
char *
nothing(ItemKind type)
{
	char *sp;
	const char *tystr;

	sprintf(prbuf, "Haven't discovered anything");
	if (game().options.terse)
		sprintf(prbuf,"Nothing");
	sp = &prbuf[strlen(prbuf)];
	switch (type)
	{
		case ItemKind::Potion: tystr = "potion"; break;
		case ItemKind::Scroll: tystr = "scroll"; break;
		case ItemKind::Ring: tystr = "ring"; break;
		case ItemKind::Stick: tystr = "stick"; break;
		//@ not in original, avoid possibly uninitialized use of tystr
		default: tystr = "item";
	}
	sprintf(sp, " about any %ss", tystr);
	return prbuf;
}

}  // namespace rogue::items
