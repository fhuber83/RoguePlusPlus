/*
 * All the fighting gets done here
 *
 * @(#)fight.c		1.43 (AI Design)		1/19/85
 */

#include "rogue.h"

namespace rogue::rules {

static bool	roll_em(Creature *thatt, Creature *thdef, Item *weap, bool hurl);
static std::string	prname(const char *who, bool upper);
static void	hit(const char *er, const char *ee);
static void	miss(const char *er, const char *ee);
static void	thunk(Item *weap, const char *mname, const char *does, const char *did);
static void	remove_monster(coord *mp, Creature *tp, bool waskill);
static int	str_plus(str_t str);
static int	add_dam(str_t str);

/*
 * fight:
 *	The player attacks the monster.
 */
bool
fight(coord *mp, char mn, Item *weap, bool thrown)
{
	Creature *tp;
	const char *mname;
	rogue::Player &player = game().player;

	/*
	 * Find the monster we want to fight
	 */
	if ((tp = moat(mp->y, mp->x)) == 0)
		return false;
	/*
	 * Since we are fighting, things are not quiet so no healing takes
	 * place.  Cancel any command counts so player can recover.
	 */
	game().turn.count = player.quiet = 0;
	start_run(mp);
	/*
	 * Let him know it was really a mimic (if it was one).
	 */
	if (tp->t_type == 'X' && tp->t_disguise != 'X' && !player.body.t_flags.test(ISBLIND)) {
		mn = tp->t_disguise = 'X';
		if (thrown)
			return false;
		msg("wait! That's a Xeroc!");
	}
	mname = monsters[mn-'A'].m_name;
	if (player.body.t_flags.test(ISBLIND))
		mname = it;
	if (roll_em(&player.body, tp, weap, thrown)||(weap && weap->o_type == ItemKind::Potion)) {
		bool did_huh = false;

		if (thrown)
			thunk(weap, mname, "hits", "hit");
		else
			hit(nullptr, mname);
		// original missed null check for weap
		if (weap && weap->o_type == ItemKind::Potion) {
			th_effect(weap, tp);
			if (!thrown) {
				if (weap->o_count > 1)
					weap->o_count--;
				else {
					player.body.t_pack.remove(weap);
					discard(weap);
				}
				player.weapon = nullptr;
			}
		}
		if (player.body.t_flags.test(CANHUH)) {
			did_huh = true;
			tp->t_flags.set(ISHUH);
			player.body.t_flags.unset(CANHUH);
			msg("your hands stop glowing red");
		}
		if (tp->t_stats.s_hpt <= 0)
			killed(tp, true);
		else if (did_huh && !player.body.t_flags.test(ISBLIND))
			msg("the {} appears confused", mname);
		return true;
	}
	if (thrown)
		thunk(weap, mname, "misses", "missed");
	else
		miss(nullptr, mname);
	if (tp->t_type == 'S' && rnd(100) > 25)
		slime_split(tp);
	return false;
}

/*
 * attack:
 *	The monster attacks the player
 */
void
attack(Creature *mp)
{
	const char *mname;
	rogue::Player &player = game().player;

	/*
	 * Since this is an attack, stop running and any healing that was
	 * going on at the time.
	 */
	game().turn.running = false;
	game().turn.count = player.quiet = 0;
	if (mp->t_type == 'X' && !player.body.t_flags.test(ISBLIND))
		mp->t_disguise = 'X';
	mname = monsters[mp->t_type-'A'].m_name;
	if (player.body.t_flags.test(ISBLIND))
		mname = it;
	if (roll_em(mp, &player.body, nullptr, false)) {
		hit(mname, nullptr);
		if (player.body.t_stats.s_hpt <= 0)
			death(mp->t_type);	/* Bye bye life ... */
		if (!mp->t_flags.test(ISCANC))
			switch (mp->t_type)
		{
		case 'A':
			/*
			 * If a rust monster hits, you lose armor, unless
			 * that armor is leather or there is a magic ring
			 */
			if (player.armor != nullptr && player.armor->o_ac < 9
			  && player.armor->which<ArmorType>() != ArmorType::Leather)
			{
				if (player.wears(Ring::MaintainArmor))
					msg("the rust vanishes instantly");
				else
				{
					msg("your armor weakens, oh my!");
					player.armor->o_ac++;
				}
			}
			break;
		case 'I':
			/*
			 * When an Ice Monster hits you, you get unfrozen faster
			 */
			if (player.no_command > 1)
				player.no_command--;
			break;
		case 'R':
			/*
			 * Rattlesnakes have poisonous bites
			 */
			if (!save(SaveThrow::Poison))
			{
				if (!player.wears(Ring::SustainStrength))
				{
					chg_str(-1);
					msg("you feel a bite in your leg{}",
						noterse(" and now feel weaker"));
				}
				else
					msg("a bite momentarily weakens you");
			}
			break;
		case 'W':
		case 'V':
			/*
			 * Wraiths might drain energy levels, and Vampires
			 * can steal maximum hit points
			 */
			if (rnd(100) < (mp->t_type == 'W' ? 15 : 30))
			{
			int fewer;

			if (mp->t_type == 'W')
			{
				if (player.body.t_stats.s_exp == 0)
				death('W');		/* All levels gone */
				if (--player.body.t_stats.s_lvl == 0)
				{
				player.body.t_stats.s_exp = 0;
				player.body.t_stats.s_lvl = 1;
				}
				else
				player.body.t_stats.s_exp = e_levels[player.body.t_stats.s_lvl-1]+1;
				fewer = roll(1, 10);
			}
			else
				fewer = roll(1, 5);
			player.body.t_stats.s_hpt -= fewer;
			player.body.t_stats.s_maxhp -= fewer;
			if (player.body.t_stats.s_hpt < 1)
				player.body.t_stats.s_hpt = 1;
			if (player.body.t_stats.s_maxhp < 1)
				death(mp->t_type);
			msg("you suddenly feel weaker");
			}
			break;
		case 'F':
			/*
			 * Violet fungi stops the poor guy from moving
			 */
			player.body.t_flags.set(ISHELD);
			// cut to fit
			*std::format_to_n(player.flytrap_damage, sizeof player.flytrap_damage - 1,
				"{}d1", ++player.fung_hit).out = '\0';
			break;
		case 'L':
		{
			/*
			 * Leperachaun steals some gold
			 */
			long lastpurse;

			lastpurse = player.purse;
			player.purse -= gold_calc();
			if (!save(SaveThrow::Magic))
			player.purse -= gold_calc() + gold_calc() + gold_calc() + gold_calc();
			if (player.purse < 0)
			player.purse = 0;
			remove_monster(&mp->t_pos, mp, false);
			if (player.purse != lastpurse)
			msg("your purse feels lighter");
		}
			break;
		case 'N':
		{
			Item *obj, *steal;
			int nobj;
			constexpr const char *she_stole = "she stole {}!";

			/*
			 * Nymph's steal a magic item, look through the pack
			 * and pick out one we like.
			 */
			steal = nullptr;
			for (nobj = 0, obj = player.body.t_pack.first(); obj != nullptr; obj = player.body.t_pack.after(obj))
			if (obj != player.armor && obj != player.weapon
				&& obj != player.rings[Hand::Left] && obj != player.rings[Hand::Right]
				&& is_magic(obj) && rnd(++nobj) == 0)
				steal = obj;
			if (steal != nullptr)
			{
				remove_monster(&mp->t_pos, mp, false);
				player.in_pack--;
				if (steal->o_count > 1 && steal->o_group == 0)
				{
					int oc;

					oc = steal->o_count--;
					steal->o_count = 1;
					msg(she_stole, inv_name(steal, true));
					steal->o_count = oc;
				}
				else
				{
					// inv_name() must run before discard() frees steal
					std::string name = inv_name(steal, true);
					player.body.t_pack.remove(steal);
					discard(steal);
					msg(she_stole, name);
				}
			}
		}
			break;
		default:
			break;
		}
	}
	else if (mp->t_type != 'I')
	{
	if (mp->t_type == 'F')
	{
		player.body.t_stats.s_hpt -= player.fung_hit;
		if (player.body.t_stats.s_hpt <= 0)
		death(mp->t_type);	/* Bye bye life ... */
	}
	miss(mname, nullptr);
	}
	flush_type();
	game().turn.count = 0;
	status();
}

/*
 * swing:
 *	Returns true if the swing hits
 */
bool
swing(int at_lvl, int op_arm, int wplus)
{
	int res = rnd(20);
	int need = (20 - at_lvl) - op_arm;

	return (res + wplus >= need);
}

/*
 * check_level:
 *	Check to see if the guy has gone up a level.
 */
void
check_level(void)
{
	int i, add, olevel;
	rogue::Player &player = game().player;

	for (i = 0; e_levels[i] != 0; i++)
	if (e_levels[i] > player.body.t_stats.s_exp)
		break;
	i++;
	olevel = player.body.t_stats.s_lvl;
	player.body.t_stats.s_lvl = i;
	if (i > olevel)
	{
		add = roll(i - olevel, 10);
		player.body.t_stats.s_maxhp += add;
		if ((player.body.t_stats.s_hpt += add) > player.body.t_stats.s_maxhp)
			player.body.t_stats.s_hpt = player.body.t_stats.s_maxhp;
		msg("and achieve the rank of \"{}\"", he_man[i-1]);
	}
}

/*
 * roll_em:
 *	Roll several attacks
 */
static bool
roll_em(Creature *thatt, Creature *thdef, Item *weap, bool hurl)
{
	rogue::Player &player = game().player;
	struct stats *att, *def;
	const char *cp;
	int def_arm;
	bool did_hit = false;
	int hplus;
	int dplus;
	int damage;
	att = &thatt->t_stats;
	def = &thdef->t_stats;
	if (weap == nullptr)
	{
		cp = att->s_dmg;
		dplus = 0;
		hplus = 0;
	}
	else
	{
		hplus = weap->o_hplus;
		dplus = weap->o_dplus;
		/*
		 * Check for vorpally enchanted weapon
		 */
		if (thdef->t_type == weap->o_enemy)
		{
			hplus += 4;
			dplus += 4;
		}
		if (weap == player.weapon)
		{
			if (player.wears(Hand::Left, Ring::IncreaseDamage))
				dplus += player.rings[Hand::Left]->o_ac;
			else if (player.wears(Hand::Left, Ring::Dexterity))
				hplus += player.rings[Hand::Left]->o_ac;
			if (player.wears(Hand::Right, Ring::IncreaseDamage))
				dplus += player.rings[Hand::Right]->o_ac;
			else if (player.wears(Hand::Right, Ring::Dexterity))
				hplus += player.rings[Hand::Right]->o_ac;
		}
		cp = weap->o_damage;
		if (hurl && weap->o_flags.test(ISMISL) && player.weapon != nullptr &&
			  launched_by(player.weapon->which<WeaponType>()) == weap->o_launch)
		{
			cp = weap->o_hurldmg;
			hplus += player.weapon->o_hplus;
			dplus += player.weapon->o_dplus;
		}
		/*
		 * Drain a staff of striking
		 */
		if (weap->o_type == ItemKind::Stick && weap->which<Stick>() == Stick::Striking
			&& --weap->charges() < 0)
		{
			cp = weap->o_damage = "0d0";
			weap->o_hplus = weap->o_dplus = 0;
			weap->charges() = 0;
		}
	}

	// New null check to prevent segfault on parsing
	if (cp == nullptr)
	{
		return false;
	}

	/*
	 * If the creature being attacked is not running (alseep or held)
	 * then the attacker gets a plus four bonus to hit.
	 */
	if (!thdef->t_flags.test(ISRUN))
		hplus += 4;
	def_arm = def->s_arm;
	if (def == &player.body.t_stats)
	{
		if (player.armor != nullptr)
			def_arm = player.armor->o_ac;
		if (player.wears(Hand::Left, Ring::Protection))
			def_arm -= player.rings[Hand::Left]->o_ac;
		if (player.wears(Hand::Right, Ring::Protection))
			def_arm -= player.rings[Hand::Right]->o_ac;
	}
	for (const rogue::Dice &attack : rogue::parse_attacks(cp))
	{
		if (swing(att->s_lvl, def_arm, hplus + str_plus(att->s_str)))
		{
			damage = dplus + attack.roll(rogue::rng()) + add_dam(att->s_str);
			/*
			 * special goodies for the commercial version of rogue
			 */
				if (thdef == &player.body && player.max_level == 1)
				 /*
				  * make it easier on level one
				  */
						damage = (damage+1) / 2;
			def->s_hpt -= std::max(0, damage);
			did_hit = true;
		}
	}
	return did_hit;
}

/*
 * prname:
 *	The print name of a combatant
 */
static std::string
prname(const char *who, bool upper)
{
	std::string name;

	if (who == 0)
		name = you;
	else if (game().player.body.t_flags.test(ISBLIND))
		name = it;
	else
		name = std::string("the ") + who;
	if (upper && !name.empty())
		name[0] = toupper(name[0]);
	return name;
}

/*
 * hit:
 *	Print a message to indicate a succesful hit
 */
static void
hit(const char *er, const char *ee)
{
	const char *s = "";

	addmsg("{}", prname(er, true));
	switch (game().options.brief() ? 1 : rnd(4))
	{
		case 0: s = " scored an excellent hit on "; break;
		case 1: s = " hit "; break;
		case 2: s = (er == 0 ? " have injured " : " has injured "); break;
		case 3: s = (er == 0 ? " swing and hit " : " swings and hits ");
		break;
	}
	msg("{}{}",s,prname(ee, false));
}

/*
 * miss:
 *	Print a message to indicate a poor swing
 */
static void
miss(const char *er, const char *ee)
{
	const char *s = "";


	addmsg("{}", prname(er, true));
	switch (game().options.brief() ? 1 : rnd(4))
	{
		case 0: s = (er == 0 ? " swing and miss" : " swings and misses"); break;
		case 1: s = (er == 0 ? " miss" : " misses"); break;
		case 2: s = (er == 0 ? " barely miss" : " barely misses"); break;
		case 3: s = (er == 0 ? " don't hit" : " doesn't hit");
		break;
	}
	msg("{} {}",s,prname(ee, false));
}

/*
 * save_throw:
 *	See if a creature save against something
 */
static bool
throw_against(int which, Creature *tp)
{
	int need;

	need = 14 + which - tp->t_stats.s_lvl / 2;
	return (roll(1, 20) >= need);
}

bool
save_throw(SaveThrow which, Creature *tp)
{
	return throw_against(std::to_underlying(which), tp);
}

/*
 * save:
 *	See if he saves against various nasty things
 */
bool
save(SaveThrow which)
{
	int against = std::to_underlying(which);

	if (which == SaveThrow::Magic) {
		if (game().player.wears(Hand::Left, Ring::Protection))
			against -= game().player.rings[Hand::Left]->o_ac;
		if (game().player.wears(Hand::Right, Ring::Protection))
			against -= game().player.rings[Hand::Right]->o_ac;
	}
	return throw_against(against, &game().player.body);
}

/*
 * str_plus:
 *	Compute bonus/penalties for strength on the "to hit" roll
 */
static int
str_plus(str_t str)
{
	int add = 4;

	if (str < 8)
		return str - 7;
	if (str < 31)
		add--;
	if (str < 21)
		add--;
	if (str < 19)
		add--;
	if (str < 17)
		add--;
	return add;
}

/*
 * add_dam:
 *	Compute additional damage done for exceptionally high or low strength
 */
static int
add_dam(str_t str)
{
	int add = 6;

	if (str < 8)
		return str - 7;
	if (str < 31)
		add--;
	if (str < 22)
		add--;
	if (str < 20)
		add--;
	if (str < 18)
		add--;
	if (str < 17)
		add--;
	if (str < 16)
		add--;
	return add;
}

/*
 * raise_level:
 *	The guy just magically went up a level.
 */
void
raise_level(void)
{
	rogue::Player &player = game().player;

	player.body.t_stats.s_exp = e_levels[player.body.t_stats.s_lvl-1] + 1L;
	check_level();
}

/*
 * thunk:
 *	A missile hit or missed a monster
 */
static void
thunk(Item *weap, const char *mname, const char *does, const char *did)
{
	if (weap->o_type == ItemKind::Weapon)
		addmsg("the {} {} ", w_names[weap->which<WeaponType>()], does);
	else
		addmsg("you {} ", did);
	if (game().player.body.t_flags.test(ISBLIND))
		msg("{}", it);
	else
		msg("the {}", mname);
}

/*
 * remove_monster:
 *	Remove a monster from the screen
 */
static void
remove_monster(coord *mp, Creature *tp, bool waskill)
{
	Item *obj, *nexti;
	TileStyle style;

	if (tp == nullptr)
		return;

	for (obj = tp->t_pack.first(); obj != nullptr; obj = nexti)
	{
		nexti = tp->t_pack.after(obj);
		obj->o_pos = tp->t_pos;
		tp->t_pack.remove(obj);
		if (waskill)
			fall(obj, false);
		else
			discard(obj);
	}
	style = (game().level.map[INDEX(mp->y,mp->x)] == PASSAGE) ? TileStyle::Inverse : TileStyle::Normal;
	if (tp->t_oldch == FLOOR && !cansee(mp->y, mp->x))
		display().draw_tile(*mp, ' ', style);
	else if (tp->t_oldch != '@')
		display().draw_tile(*mp, tp->t_oldch, style);
	game().level.monsters.remove(tp);
	discard(tp);
}

/*
 * is_magic:
 *	Returns true if an object radiates magic
 */
bool
is_magic(Item *obj)
{
	switch (obj->o_type)
	{
	case ItemKind::Armor:
		return obj->o_ac != a_class[obj->which<ArmorType>()];
	case ItemKind::Weapon:
		return obj->o_hplus != 0 || obj->o_dplus != 0;
	case ItemKind::Potion:
	case ItemKind::Scroll:
	case ItemKind::Stick:
	case ItemKind::Ring:
	case ItemKind::Amulet:
		return true;
	default:	// the other kinds of item: nothing
		break;
	}
	return false;
}

/*
 * killed:
 *	Called to put a monster to death
 */
void
killed(Creature *tp, bool pr)
{
	char type = tp->t_type;	// remove_monster() discards tp

	game().player.body.t_stats.s_exp += tp->t_stats.s_exp;
	/*
	 * If the monster was a violet fungi, un-hold him
	 */
	switch (tp->t_type)
	{
	case 'F':
		game().player.body.t_flags.unset(ISHELD);
		f_restor();
		break;
	case 'L':;
		Item *gold;

		if ((gold = new_item()) == nullptr)
			return;
		gold->o_type = ItemKind::Gold;
		gold->gold_value() = gold_calc();
		if (save(SaveThrow::Magic))
			gold->gold_value() += gold_calc() + gold_calc() + gold_calc() + gold_calc();
		tp->t_pack.push_front(gold);
		break;
	}
	/*
	 * Get rid of the monster.
	 */
	remove_monster(&tp->t_pos, tp, true);
	if (pr)
	{
	addmsg("you have defeated ");
	if (game().player.body.t_flags.test(ISBLIND))
		msg("{}", it);
	else
		msg("the {}", monsters[type-'A'].m_name);
	}
	/*
	 * Do adjustments if he went up a level
	 */
	check_level();
}

}  // namespace rogue::rules
