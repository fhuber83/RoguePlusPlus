/*
 * global variable initializaton
 *
 * init.c	1.4 (A.I. Design) 12/14/84
 */

#include "rogue.h"


/*
 * init_player:
 *	Roll up the rogue
 */
void
init_player()
{
	Item *obj;
	bcopy(pstats,game().player.max_stats);
	game().player.food_left = HUNGERTIME;
	/*
	 * initialize things
	 */
	game().pool = rogue::Pool();
	/*
	 * Give the rogue his weaponry.  First a mace.
	 */
	obj = new_item();
	obj->o_type = ItemKind::Weapon;
	obj->o_which = MACE;
	init_weapon(obj, MACE);
	obj->o_hplus = 1;
	obj->o_dplus = 1;
	obj->o_flags.set(ISKNOW);
	obj->o_count = 1;
	obj->o_group = 0;
	add_pack(obj, TRUE);
	game().player.weapon = obj;
	/*
	 * Now a +1 bow
	 */
	obj = new_item();
	obj->o_type = ItemKind::Weapon;
	obj->o_which = BOW;
	init_weapon(obj, BOW);
	obj->o_hplus = 1;
	obj->o_dplus = 0;
	obj->o_count = 1;
	obj->o_group = 0;
	obj->o_flags.set(ISKNOW);
	add_pack(obj, TRUE);
	/*
	 * Now some arrows
	 */
	obj = new_item();
	obj->o_type = ItemKind::Weapon;
	obj->o_which = ARROW;
	init_weapon(obj, ARROW);
	obj->o_count = rnd(15) + 25;
	obj->o_hplus = obj->o_dplus = 0;
	obj->o_flags.set(ISKNOW);
	add_pack(obj, TRUE);
	/*
	 * And his suit of armor
	 */
	obj = new_item();
	obj->o_type = ItemKind::Armor;
	obj->o_which = RING_MAIL;
	obj->o_ac = a_class[RING_MAIL] - 1;
	obj->o_flags.set(ISKNOW);
	obj->o_count = 1;
	obj->o_group = 0;
	game().player.armor = obj;
	add_pack(obj, TRUE);
	/*
	 * Give him some food too
	 */
	obj = new_item();
	obj->o_type = ItemKind::Food;
	obj->o_count = 1;
	obj->o_which = 0;
	obj->o_group = 0;
	add_pack(obj, TRUE);
}

/*
 * Contains definitions and functions for dealing with things like
 * potions and scrolls
 */

static const char *rainbow[] = {
	"amber",
	"aquamarine",
	"black",
	"blue",
	"brown",
	"clear",
	"crimson",
	"cyan",
	"ecru",
	"gold",
	"green",
	"grey",
	"magenta",
	"orange",
	"pink",
	"plaid",
	"purple",
	"red",
	"silver",
	"tan",
	"tangerine",
	"topaz",
	"turquoise",
	"vermilion",
	"violet",
	"white",
	"yellow"
};

#define NCOLORS (sizeof rainbow / sizeof (char *))

static const char *c_set = "bcdfghjklmnpqrstvwxyz";
static const char *v_set = "aeiou";

typedef struct {
	const char	*st_name;
	int		st_value;
} STONE;

static STONE stones[] = {
	{ "agate",		 25},
	{ "alexandrite",	 40},
	{ "amethyst",	 50},
	{ "carnelian",	 40},
	{ "diamond",	300},
	{ "emerald",	300},
	{ "germanium",	225},
	{ "granite",	  5},
	{ "garnet",		 50},
	{ "jade",		150},
	{ "kryptonite",	300},
	{ "lapis lazuli",	 50},
	{ "moonstone",	 50},
	{ "obsidian",	 15},
	{ "onyx",		 60},
	{ "opal",		200},
	{ "pearl",		220},
	{ "peridot",	 63},
	{ "ruby",		350},
	{ "sapphire",	285},
	{ "stibotantalite",	200},
	{ "tiger eye",	 50},
	{ "topaz",		 60},
	{ "turquoise",	 70},
	{ "taaffeite",	300},
	{ "zircon",	 	 80}
};

#define NSTONES (sizeof stones / sizeof (STONE))

static const char *wood[] = {
	"avocado wood",
	"balsa",
	"bamboo",
	"banyan",
	"birch",
	"cedar",
	"cherry",
	"cinnibar",
	"cypress",
	"dogwood",
	"driftwood",
	"ebony",
	"elm",
	"eucalyptus",
	"fall",
	"hemlock",
	"holly",
	"ironwood",
	"kukui wood",
	"mahogany",
	"manzanita",
	"maple",
	"oaken",
	"persimmon wood",
	"pecan",
	"pine",
	"poplar",
	"redwood",
	"rosewood",
	"spruce",
	"teak",
	"walnut",
	"zebrawood"
};

#define NWOOD (sizeof wood / sizeof (char *))

static const char *metal[] = {
	"aluminum",
	"beryllium",
	"bone",
	"brass",
	"bronze",
	"copper",
	"electrum",
	"gold",
	"iron",
	"lead",
	"magnesium",
	"mercury",
	"nickel",
	"pewter",
	"platinum",
	"steel",
	"silver",
	"silicon",
	"tin",
	"titanium",
	"tungsten",
	"zinc"
};

#define NMETAL (sizeof metal / sizeof (char *))

/*
 * init_things
 *	Initialize the probabilities for types of things
 */
void
init_things()
{
	struct magic_item *mp;

	for (mp = &game().items.things[1]; mp <= &game().items.things[NUMTHINGS-1]; mp++)
		mp->mi_prob += (mp-1)->mi_prob;
}

/*
 * init_colors:
 *	Initialize the potion color scheme for this time
 */
void
init_colors()
{
	unsigned int i, j;
	bool used[NCOLORS];
	rogue::Items &items = game().items;

	for (i = 0; i < NCOLORS; i++)
		used[i] = FALSE;
	for (i = 0; i < MAXPOTIONS; i++)
	{
		do
			j = rnd(NCOLORS);
		while (used[j]);
		used[j] = TRUE;
		items.p_colors[i] = rainbow[j];
		items.p_know[i] = FALSE;
		items.p_guess[i] = (char *)&items.guesses[items.iguess++];
		if (i > 0)
			items.p_magic[i].mi_prob += items.p_magic[i-1].mi_prob;
	}
}

/*
 * init_names:
 *	Generate the names of the various scrolls
 */
void
init_names()
{
	rogue::Items &items = game().items;
	 int nsyl;
	 char *cp, *sp;
	 int i, nwords;

	for (i = 0; i < MAXSCROLLS; i++)
	{
	cp = prbuf;
	nwords = rnd(game().options.terse?3:4) + 2;
	while (nwords--)
	{
		nsyl = rnd(2) + 1;
		while (nsyl--)
		{
		sp = getsyl();
		if (&cp[strlen(sp)] > &prbuf[MAXNAME-1])
		{
			nwords = 0;
			break;
		}
		while (*sp)
			*cp++ = *sp++;
		}
		*cp++ = ' ';
	}
	*--cp = '\0';
	/*
	 * I'm tired of thinking about this one so just in case .....
	 */
	prbuf[MAXNAME] = 0;
	items.s_know[i] = FALSE;
	items.s_guess[i] = (char *)&items.guesses[items.iguess++];
	strcpy((char *)(&items.s_names[i]), prbuf);
	if (i > 0)
		items.s_magic[i].mi_prob += items.s_magic[i-1].mi_prob;
	}
}

/*
 * getsyl()
 *   -- generate a random sylable
 */
char*
getsyl()
{
	static char _tsyl[4];

	_tsyl[3] = 0;
	_tsyl[2] = rchr(c_set);
	_tsyl[1] = rchr(v_set);
	_tsyl[0] = rchr(c_set);
	return (_tsyl);
}

/*
 * rchr()
 *    return random character in given string
 */
char
rchr(const char *string)
{
	return(string[rnd(strlen(string))]);
}

/*
 * init_stones:
 *	Initialize the ring stone setting scheme for this time
 */
void
init_stones()
{
	unsigned int i, j;
	bool used[NSTONES];
	rogue::Items &items = game().items;

	for (i = 0; i < NSTONES; i++)
		used[i] = FALSE;
	for (i = 0; i < MAXRINGS; i++)
	{
		do
			j = rnd(NSTONES);
		while (used[j]);
		used[j] = TRUE;
		items.r_stones[i] = stones[j].st_name;
		items.r_know[i] = FALSE;
		items.r_guess[i] = (char *)&items.guesses[items.iguess++];
		if (i > 0)
			items.r_magic[i].mi_prob += items.r_magic[i-1].mi_prob;
		items.r_magic[i].mi_worth += stones[j].st_value;
	}
}

/*
 * init_materials:
 *	Initialize the construction materials for wands and staffs
 */
void
init_materials()
{
	unsigned int i, j;
	const char *str;
	bool metused[NMETAL], woodused[NWOOD];
	rogue::Items &items = game().items;

	for (i = 0; i < NWOOD; i++)
		woodused[i] = FALSE;
	for (i = 0; i < NMETAL; i++)
		metused[i] = FALSE;
	for (i = 0; i < MAXSTICKS; i++)
	{
		for (;;)
			if (rnd(2) == 0)
			{
				j = rnd(NMETAL);
				if (!metused[j])
				{
					items.ws_type[i] = "wand";
					str = metal[j];
					metused[j] = TRUE;
					break;
				}
			}
			else
			{
				j = rnd(NWOOD);
				if (!woodused[j])
				{
					items.ws_type[i] = "staff";
					str = wood[j];
					woodused[j] = TRUE;
					break;
				}
			}
		items.ws_made[i] = str;
		items.ws_know[i] = FALSE;
		items.ws_guess[i] = (char *)&items.guesses[items.iguess++];
		if (i > 0)
			items.ws_magic[i].mi_prob += items.ws_magic[i-1].mi_prob;
	}
}

/*
 * Declarations for allocated things
 */
long *e_levels;		/* Pointer to array of experience level */
char *tbuf;			/* Temp buffer used in fighting */
char *prbuf;		/* Printing buffer used everywhere */
char *ring_buf;		/* Buffer used by ring code */
//@ Deprecated:
//@ char *end_mem;	/* Pointer to end of memory */


/*
 *  Declarations for data space that must be saved and restored exaxtly
 */

/*
 * init_ds()
 *   Allocate things data space
 */
void
init_ds(void)
{
	long *ep;

	/*@
	 * Do not change the relation between the allocated pointer and its
	 * associated size constant! If the sizes need to be changed, do so by
	 * altering the value in the #define'd constant. For example, prbuf is
	 * expected to have a MAXSTR size, but MAXSTR can be re-#define'd to
	 * another value. Also, for safety, never decrease its value.
	 */

	//@ data that is saved to and restored from saved game files:

	//@ data discarded and re-created on new and restored games:
	tbuf = newmem(MAXSTR);
	prbuf = newmem(MAXSTR);
	ring_buf = newmem(6);
	e_levels = (long *)newmem(20 * sizeof (long));
	for (ep = e_levels+1, *e_levels = 10L; ep < e_levels + 19; ep++)
		*ep = *(ep-1) << 1;
	*ep = 0L;
}


void
free_ds()
{
	free(tbuf);
	free(prbuf);
	free(ring_buf);
	free(e_levels);
}
