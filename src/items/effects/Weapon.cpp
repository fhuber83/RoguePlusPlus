#include <chrono>
#include <thread>
#include <variant>

#include "rogue.h"

namespace rogue::items::effects {

constexpr char NONE = 100;

struct init_weps {
	rogue::Attacks iw_dam;	/* Damage when wielded */
	rogue::Attacks iw_hrl;	/* Damage when thrown */
	char iw_launch;	/* Launching weapon */
	ItemFlags iw_flags;	/* Miscellaneous flags */
};

static constexpr KindTable<WeaponType, init_weps> init_dam = {
	{"2d4",	"1d3",	NONE,     {}},            	/* Mace */
	{"3d4",	"1d2",	NONE,     {}},            	/* Long sword */
	{"1d1",	"1d1",	NONE,     {}},            	/* Bow */
	{"1d1",	"2d3",	launched_by(WeaponType::ShortBow), ISMANY|ISMISL},	/* Arrow */
	{"1d6",	"1d4",	NONE,     ISMISL},       	/* Dagger */
	{"4d4",	"1d2",	NONE,     {}},            	/* 2h sword */
	{"1d1",	"1d3",	NONE,     ISMANY|ISMISL},	/* Dart */
	{"1d1",	"1d1",	NONE,     {}},            	/* Crossbow */
	{"1d2",	"2d5",	launched_by(WeaponType::Crossbow), ISMANY|ISMISL},	/* Crossbow bolt */
	{"2d3",	"1d6",	NONE,     ISMISL}        	/* Spear */
};

// Where fallpos() puts an item: nowhere, a free spot, or a pile it joined
struct JoinedPile {};
using Landing = std::variant<std::monostate, Coord, JoinedPile>;

static Landing	fallpos(Item *obj);
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
	if ((obj = get_item("throw", ItemKind::Weapon)) == nullptr)
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
		game().player.body.t_pack.remove(obj);
		game().player.in_pack--;
	} else {
		/*
		 * here is a quick hack to check if we can get a new item
		 */
		if ((nitem = new_item()) == nullptr) {
			obj->o_count = 1;
			msg("something in your pack explodes!!!");
			goto hack;
		}
		obj->o_count--;
		if (obj->o_group == 0)
			game().player.in_pack--;
		*nitem = *obj;
		nitem->o_count = 1;
		obj = nitem;
	}
	do_motion(obj, ydelta, xdelta);
	/*
	 * AHA! Here it has hit something.  If it is a wall or a door,
	 * or if it misses (combat) the monster, put it on the floor
	 */
	if (moat(obj->o_pos.y, obj->o_pos.x) == nullptr
		|| !hit_monster(obj->o_pos.y, obj->o_pos.x, obj))
			fall(obj, true);
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
	rogue::Player &player = game().player;

	/*
	 * Come fly with us ...
	 */
	obj->o_pos = player.body.t_pos;
	for (;;) {
		int ch;

		/*
		 * Erase the old one
		 */
		if (under != '@' && !(obj->o_pos == player.body.t_pos) && cansee(obj->o_pos.y, obj->o_pos.x))
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
			if (cansee(obj->o_pos.y, obj->o_pos.x)) {
				under = game().level.at(obj->o_pos);
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
		case ItemKind::Weapon: return std::string(w_names[obj->which<WeaponType>()]);
		case ItemKind::Armor: return std::string(a_names[obj->which<ArmorType>()]);
		case ItemKind::Food: return "food";
		case ItemKind::Potion:
		case ItemKind::Scroll:
		case ItemKind::Amulet:
		case ItemKind::Stick:
		case ItemKind::Ring:
		{
			std::string name = inv_name(obj, true);
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
	int index;
	rogue::Level &level = game().level;
	Landing landing = fallpos(obj);

	if (std::holds_alternative<Coord>(landing))
	{
		const Coord fpos = std::get<Coord>(landing);

		index = INDEX(fpos.y, fpos.x);
		level.map[index] = glyph_of(obj->o_type);
		obj->o_pos = fpos;
		if (cansee(fpos.y, fpos.x))
		{
			display().draw_tile(fpos, glyph_of(obj->o_type),
					(level.flags_at(obj->o_pos).test(MapFlag::Passage) ||
					 level.flags_at(obj->o_pos).test(MapFlag::Maze))
						? TileStyle::Inverse : TileStyle::Normal);
			if (moat(fpos.y,fpos.x) != nullptr)
				moat(fpos.y,fpos.x)->t_oldch = glyph_of(obj->o_type);
		}
		level.objects.push_front(obj);
		return;
	}
	if (std::holds_alternative<JoinedPile>(landing))
		pr = false;
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
init_weapon(Item *weap, WeaponType type)
{
	const struct init_weps *iwp;

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
	Creature *mo = moat(y, x);

	if (mo)
		return fight({x, y}, mo->t_type, obj, true);
	return false;
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

	oweapon = player.weapon_item();
	if (!can_drop(player.weapon_item()))
	{
		player.weapon = game().pool.id_of(oweapon);
		return;
	}
	player.weapon = game().pool.id_of(oweapon);
	if ((obj = get_item("wield", ItemKind::Weapon)) == nullptr)
	{
bad:
		game().turn.after = false;
		return;
	}

	if (obj->o_type == ItemKind::Armor)
	{
		msg("you can't wield armor");
		goto bad;
	}
	if (is_current(obj))
		goto bad;

	sp = inv_name(obj, true);
	player.weapon = game().pool.id_of(obj);
	ifterse("now wielding {} ({:c})", "you are now wielding {} ({:c})",
		sp, pack_char(obj));
}

/*
 * fallpos:
 *	Pick a random position around the given (y, x) coordinates
 */
static
Landing
fallpos(Item *obj)
{
	int y, x, cnt = 0, ch;
	Coord newpos;
	Item *onfloor;
	rogue::Player &player = game().player;

	for (y = obj->o_pos.y - 1; y <= obj->o_pos.y + 1; y++) {
		for (x = obj->o_pos.x - 1; x <= obj->o_pos.x + 1; x++) {
			/*
			 * check to make certain the spot is empty, if it is,
			 * put the object there, set it in the level list
			 * and re-draw the room if he can see it
			 */
			if ((y == player.body.t_pos.y && x == player.body.t_pos.x) || offmap(y,x))
				continue;
			if ((ch = game().level.at(y, x)) == FLOOR || ch == PASSAGE) {
				if (rnd(++cnt) == 0)
					newpos = {x, y};
				continue;
			}
			if (step_ok(ch)
				&& (onfloor = find_obj(y, x))
				&& onfloor->o_type == obj->o_type
				&& onfloor->o_group
				&& onfloor->o_group == obj->o_group)
			{
				onfloor->o_count += obj->o_count;
				return JoinedPile{};
			}
		}
	}
	if (cnt == 0)
		return std::monostate{};
	return newpos;
}


// pause for a tick, ie, 1/18.2 secs (about 55ms)
void
tick_pause(void)
{
	display().flush();
	std::this_thread::sleep_for(std::chrono::milliseconds(55));
}

}  // namespace rogue::items::effects
