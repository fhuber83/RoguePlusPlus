#include "rogue.h"

namespace rogue::items::effects {

#define NONE 100

static struct init_weps {
	const char *iw_dam;	/* Damage when wielded */
	const char *iw_hrl;	/* Damage when thrown */
	char iw_launch;	/* Launching weapon */
	ItemFlags iw_flags;	/* Miscellaneous flags */
} init_dam[MAXWEAPONS] = {
	{"2d4",	"1d3",	NONE,     {}},            	/* Mace */
	{"3d4",	"1d2",	NONE,     {}},            	/* Long sword */
	{"1d1",	"1d1",	NONE,     {}},            	/* Bow */
	{"1d1",	"2d3",	BOW,      ISMANY|ISMISL},	/* Arrow */
	{"1d6",	"1d4",	NONE,     ISMISL},       	/* Dagger */
	{"4d4",	"1d2",	NONE,     {}},            	/* 2h sword */
	{"1d1",	"1d3",	NONE,     ISMANY|ISMISL},	/* Dart */
	{"1d1",	"1d1",	NONE,     {}},            	/* Crossbow */
	{"1d2",	"2d5",	CROSSBOW, ISMANY|ISMISL},	/* Crossbow bolt */
	{"2d3",	"1d6",	NONE,     ISMISL}        	/* Spear */
};

static int	fallpos(Item *obj, coord *newpos);
static std::string	short_name(Item *obj);

/*
 * missile:
 *	Fire a missile in a given direction
 */
void
missile(int ydelta, int xdelta)
{
	Item *obj, *nitem;

	/*
	 * Get which thing we are hurling
	 */
	if ((obj = get_item("throw", ItemKind::Weapon)) == NULL)
		return;
	if (!can_drop(obj) || is_current(obj))
		return;
	/*
	 * Get rid of the thing.  If it is a non-multiple item object, or
	 * if it is the last thing, just drop it.  Otherwise, create a new
	 * item with a count of one.
	 */
	hack:
	if (obj->o_count < 2) {
		detach(pack, obj);
		game().player.in_pack--;
	} else {
		/*
		 * here is a quick hack to check if we can get a new item
		 */
		if ((nitem = new_item()) == NULL) {
			obj->o_count = 1;
			msg("something in your pack explodes!!!");
			goto hack;
		}
		obj->o_count--;
		if (obj->o_group == 0)
			game().player.in_pack--;
		bcopy(*nitem,*obj);
		nitem->o_count = 1;
		obj = nitem;
	}
	do_motion(obj, ydelta, xdelta);
	/*
	 * AHA! Here it has hit something.  If it is a wall or a door,
	 * or if it misses (combat) the monster, put it on the floor
	 */
	if (moat(obj->o_pos.y, obj->o_pos.x) == NULL
		|| !hit_monster(unc(obj->o_pos), obj))
			fall(obj, TRUE);
}

/*
 * do_motion:
 *	Do the actual motion on the screen done by an object traveling
 *	across the room
 */
void
do_motion(Item *obj, int ydelta, int xdelta)
{
	unsigned char under = '@';

	/*
	 * Come fly with us ...
	 */
	bcopy(obj->o_pos,hero);
	for (;;) {
		int ch;

		/*
		 * Erase the old one
		 */
		if (under != '@' && !(obj->o_pos == hero) && cansee(unc(obj->o_pos)))
			display().draw_tile(obj->o_pos, under);
		/*
		 * Get the new position
		 */
		obj->o_pos.y += ydelta;
		obj->o_pos.x += xdelta;

		if (step_ok(ch = winat(obj->o_pos.y, obj->o_pos.x)) && ch != DOOR) {
			/*
			 * It hasn't hit anything yet, so display it
			 * If it alright.
			 */
			if (cansee(unc(obj->o_pos))) {
				under = chat(obj->o_pos.y, obj->o_pos.x);
				display().draw_tile(obj->o_pos, glyph_of(obj->o_type));
				tick_pause();
			} else
				under = '@';
			continue;
		}
		break;
	}
}

static
std::string
short_name(Item *obj)
{
	switch (obj->o_type) {
		case ItemKind::Weapon: return w_names[obj->o_which];
		case ItemKind::Armor: return a_names[obj->o_which];
		case ItemKind::Food: return "food";
		case ItemKind::Potion:
		case ItemKind::Scroll:
		case ItemKind::Amulet:
		case ItemKind::Stick:
		case ItemKind::Ring:
		{
			std::string name = inv_name(obj, TRUE);
			return name.substr(name.find(' ') + 1);
		}
		default:
			return "bizzare thing";
	}
}

/*
 * fall:
 *	Drop an item someplace around here.
 */
void
fall(Item *obj, bool pr)
{
	static coord fpos;
	int index;

	switch (fallpos(obj, &fpos))
	{
	case 1:
		index = INDEX(fpos.y, fpos.x);
		game().level.map[index] = glyph_of(obj->o_type);
		bcopy(obj->o_pos,fpos);
		if (cansee(fpos.y, fpos.x))
		{
			display().draw_tile(fpos, glyph_of(obj->o_type),
					((flat(obj->o_pos.y, obj->o_pos.x) & F_PASS) ||
					 (flat(obj->o_pos.y, obj->o_pos.x) & F_MAZE))
						? TileStyle::Inverse : TileStyle::Normal);
			if (moat(fpos.y,fpos.x) != NULL)
				moat(fpos.y,fpos.x)->t_oldch = glyph_of(obj->o_type);
		}
		attach(game().level.objects, obj);
		return;
	case 2:
		pr = 0;
		break;
	}
	if (pr)
		msg("the {} vanishes{}.", short_name(obj),
								  noterse(" as it hits the ground"));
	discard(obj);
}

/*
 * init_weapon:
 *	Set up the initial goodies for a weapon
 */
void
init_weapon(Item *weap, unsigned char type)
{
	struct init_weps *iwp;

	iwp = &init_dam[type];
	weap->o_damage = iwp->iw_dam;
	weap->o_hurldmg = iwp->iw_hrl;
	weap->o_launch = iwp->iw_launch;
	weap->o_flags = iwp->iw_flags;
	if (weap->o_flags.test(ISMANY))
	{
		weap->o_count = rnd(8) + 8;
		weap->o_group = game().items.group++;
	}
	else
		weap->o_count = 1;
}

/*
 * hit_monster:
 *	Does the missile hit the monster?
 */
bool
hit_monster(int y, int x, Item *obj)
{
	static coord mp;
	Creature *mo = moat(y, x);

	if (mo) {
		mp.y = y;
		mp.x = x;
		return fight(&mp, mo->t_type, obj, TRUE);
	}
	return FALSE;
}

/*
 * num:
 *	Figure out the plus number for armor/weapons
 */
std::string
num(int n1, int n2, char type)
{
	std::string numbuf = std::format("{:+}", n1);

	if (type == WEAPON)
		numbuf += std::format(",{:+}", n2);
	return numbuf;
}

/*
 * wield:
 *	Pull out a certain weapon
 */
void
wield(void)
{
	Item *obj, *oweapon;
	std::string sp;
	rogue::Player &player = game().player;

	oweapon = player.weapon;
	if (!can_drop(player.weapon))
	{
		player.weapon = oweapon;
		return;
	}
	player.weapon = oweapon;
	if ((obj = get_item("wield", ItemKind::Weapon)) == NULL)
	{
bad:
		game().turn.after = FALSE;
		return;
	}

	if (obj->o_type == ItemKind::Armor)
	{
		msg("you can't wield armor");
		goto bad;
	}
	if (is_current(obj))
		goto bad;

	sp = inv_name(obj, TRUE);
	player.weapon = obj;
	ifterse("now wielding {} ({:c})", "you are now wielding {} ({:c})",
		sp, pack_char(obj));
}

/*
 * fallpos:
 *	Pick a random position around the given (y, x) coordinates
 */
static
int
fallpos(Item *obj, coord *newpos)
{
	int y, x, cnt = 0, ch;
	Item *onfloor;

	for (y = obj->o_pos.y - 1; y <= obj->o_pos.y + 1; y++) {
		for (x = obj->o_pos.x - 1; x <= obj->o_pos.x + 1; x++) {
			/*
			 * check to make certain the spot is empty, if it is,
			 * put the object there, set it in the level list
			 * and re-draw the room if he can see it
			 */
			if ((y == hero.y && x == hero.x) || offmap(y,x))
				continue;
			if ((ch = chat(y, x)) == FLOOR || ch == PASSAGE) {
				if (rnd(++cnt) == 0) {
					newpos->y = y;
					newpos->x = x;
				}
				continue;
			}
			if (step_ok(ch)
				&& (onfloor = find_obj(y, x))
				&& onfloor->o_type == obj->o_type
				&& onfloor->o_group
				&& onfloor->o_group == obj->o_group)
			{
				onfloor->o_count += obj->o_count;
				return 2;
			}
		}
	}
	return(cnt != 0);
}


// pause for a tick, ie, 1/18.2 secs (about 55ms)
void
tick_pause(void)
{
	display().flush();
	msleep(55);
}

}  // namespace rogue::items::effects
