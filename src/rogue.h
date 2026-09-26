/*
 * Rogue definitions and variable declarations
 *
 * rogue.h	1.4 (AI Design) 12/14/84
 */

/*
 * Modern headers first: extern.h and this file define macros such as max(),
 * pack and when that would break standard library headers.
 */
#include <format>
#include <optional>
#include <string>
#include <vector>

#include "core/Coord.hpp"
#include "core/Dice.hpp"
#include "core/Flags.hpp"
#include "core/Random.hpp"
#include "entities/List.hpp"
#include "game/Slots.hpp"
#include "ui/Display.hpp"
#include "ui/Input.hpp"

#include "extern.h"
#include "glyphs.h"

/*
 * Screen size. Fixed at 80x25 (see rogue::ui::Screen); these used to be the
 * ncurses globals of the same name. Only game files see these; the curses
 * backend uses ncurses' own.
 */
const int LINES = MAXLINES;
const int COLS = MAXCOLS;
// Last line used for the map
const int maxrow = MAXLINES - 2;


/*
 *  Options set for PC rogue
 */

/*
 * if DEBUG or WIZARD is changed
 * might as well recompile everything
 */
#define HELP
#undef DEMO
#define DEMOTIME 10
/*
 * DEMO
 *      recompile:
 *          save.c
 *	    rip.c
 *          io.c
 *          main.c
 */
#define REV 1
#define VER 48

/*
 * If CODECSUM is changed recompile extern.c
 */
#define SCOREFILE "rogue.scr"
#define SAVEFILE  "rogue.sav"
#define ENVFILE	  "rogue.opt"
#define IBM
#define MACROSZ 41

/*
 * Maximum number of different things
 */
#define MAXROOMS	9
#define MAXTHINGS	9
#define MAXOBJ		9
#define MAXPACK		23
#define MAXTRAPS	10
#define AMULETLEVEL	26
#define	NUMTHINGS	7	/* number of types of things */
#define MAXPASS		13	/* upper limit on number of passages */
#define MAXNAME		20  /* Maximum Length of a scroll */
#define MAXITEMS	83  /* Maximum number of randomly generated things */
#define BUFSIZE		128

/*
 * All the fun defines
 */
#define hero		game().player.body.t_pos
#define pstats		game().player.body.t_stats
#define pack		game().player.body.t_pack
#define proom		game().player.body.t_room
#define max_hp		game().player.body.t_stats.s_maxhp
#define attach(a,b)	(a).push_front(b)
#define detach(a,b)	(a).remove(b)
#define free_list(a)	list_free(a)
#define max(a,b)	((a) > (b) ? (a) : (b))
#define GOLDCALC	(rnd(50 + 10 * game().level.depth) + 2)
#define ISRING(h,r)	(game().player.rings[h] != NULL && game().player.rings[h]->o_which == r)
#define ISWEARING(r)	(ISRING(LEFT, r) || ISRING(RIGHT, r))
#define ISMULT(type) 	(type==ItemKind::Potion || type==ItemKind::Scroll || type==ItemKind::Food || type==ItemKind::Gold)
#define chat(y,x)	(game().level.map[INDEX(y,x)])
#define flat(y,x)	(game().level.flags[INDEX(y,x)])
#define unc(cp)		(cp).y, (cp).x
#define isfloor(c)	((c) == FLOOR || (c) == PASSAGE)
#define isgone(rp)	((rp)->r_flags.test(RoomFlag::Gone) && !(rp)->r_flags.test(RoomFlag::Maze))
#ifdef WIZARD
#define debug		if (wizard) msg
#endif
#define ismonster(ch)	(((ch) >= 'A') && ((ch) <= 'Z'))

/*
 * Various constants
 */
#define BEARTIME	spread(3)
#define SLEEPTIME	spread(5)
#define HEALTIME	spread(30)
#define HOLDTIME	spread(2)
#define WANDERTIME	spread(70)
#define HUHDURATION	spread(20)
#define SEEDURATION	spread(300)
#define HUNGERTIME	spread(1300)
#define MORETIME	150
#define STOMACHSIZE	2000
#define STARVETIME	850
#define LEFT		0
#define RIGHT		1
#define BOLT_LENGTH	6
#define LAMPDIST	3

/*
 * Save against things
 */
#define VS_POISON	00
#define VS_PARALYZATION	00
#define VS_LUCK		01
#define VS_DEATH	00
#define VS_BREATH	02
#define VS_MAGIC	03


/*
 * Flags for level map
 */
#define F_PASS		0x040		/* is a passageway */
#define F_MAZE		0x020		/* have seen this corridor before */
#define F_REAL		0x010		/* what you see is what you get */
#define F_PNUM		0x00f		/* passage number mask */
#define F_TMASK		0x007		/* trap number mask */

/*
 * Trap types
 */
#define T_DOOR	00
#define T_ARROW	01
#define T_SLEEP	02
#define T_BEAR	03
#define T_TELEP	04
#define T_DART	05
#define NTRAPS	6

/*
 * Potion types
 */
#define P_CONFUSE	0
#define P_PARALYZE	1
#define P_POISON	2
#define P_STRENGTH	3
#define P_SEEINVIS	4
#define P_HEALING	5
#define P_MFIND		6
#define	P_TFIND 	7
#define	P_RAISE		8
#define P_XHEAL		9
#define P_HASTE		10
#define P_RESTORE	11
#define P_BLIND		12
#define P_NOP		13
#define MAXPOTIONS	14

/*
 * Scroll types
 */
#define S_CONFUSE	0
#define S_MAP		1
#define S_HOLD		2
#define S_SLEEP		3
#define S_ARMOR		4
#define S_IDENT		5
#define S_SCARE		6
#define S_GFIND		7
#define S_TELEP		8
#define S_ENCH		9
#define S_CREATE	10
#define S_REMOVE	11
#define S_AGGR		12
#define S_NOP		13
#define S_VORPAL	14
#define MAXSCROLLS	15

/*
 * Weapon types
 */
#define MACE		0
#define SWORD		1
#define BOW		2
#define ARROW		3
#define DAGGER		4
#define TWOSWORD	5
#define DART		6
#define CROSSBOW	7
#define BOLT		8
#define SPEAR		9
#define FLAME		10	/* fake entry for dragon breath (ick) */
#define MAXWEAPONS	10	/* this should equal FLAME */

/*
 * Armor types
 */
#define LEATHER		0
#define RING_MAIL	1
#define STUDDED_LEATHER	2
#define SCALE_MAIL	3
#define CHAIN_MAIL	4
#define SPLINT_MAIL	5
#define BANDED_MAIL	6
#define PLATE_MAIL	7
#define MAXARMORS	8

/*
 * Ring types
 */
#define R_PROTECT	0
#define R_ADDSTR	1
#define R_SUSTSTR	2
#define R_SEARCH	3
#define R_SEEINVIS	4
#define R_NOP		5
#define R_AGGR		6
#define R_ADDHIT	7
#define R_ADDDAM	8
#define R_REGEN		9
#define R_DIGEST	10
#define R_TELEPORT	11
#define R_STEALTH	12
#define R_SUSTARM	13
#define MAXRINGS	14

/*
 * Rod/Wand/Staff types
 */

#define WS_LIGHT	0
#define WS_HIT		1
#define WS_ELECT	2
#define WS_FIRE		3
#define WS_COLD		4
#define WS_POLYMORPH	5
#define WS_MISSILE	6
#define WS_HASTE_M	7
#define WS_SLOW_M	8
#define WS_DRAIN	9
#define WS_NOP		10
#define WS_TELAWAY	11
#define WS_TELTO	12
#define WS_CANCEL	13
#define MAXSTICKS	14

/*
 * Now we define the structures and types
 */

/*
 * Help list
 */
struct h_list {
	unsigned char h_chstr[6];  // either (ch) or (ch,sep,ch2) appended with ": "
	const char *h_desc;
};

/*
 * Coordinate data type
 */
using coord = rogue::Coord;  // see core/Coord.hpp

// Game output goes through the display, see ui/Display.hpp
using rogue::ui::display;
using rogue::ui::input;
using rogue::ui::TileStyle;

/*
 * Data type for strength values and modifiers
 */
typedef unsigned int str_t;

/*
 * Stuff about magic items
 */

struct magic_item {
	const char *mi_name;
	int mi_prob;
	short mi_worth;
};

struct array {
	char storage[MAXNAME+1];
};

/*
 * Room structure
 */
namespace rogue {
enum class RoomFlag : unsigned short {
	Dark = 0x0001,	/* room is dark */
	Gone = 0x0002,	/* room is gone (a corridor) */
	Maze = 0x0004,	/* room is a maze */
};
template <>
inline constexpr bool enable_flags<RoomFlag> = true;
}  // namespace rogue
using rogue::RoomFlag;
using RoomFlags = rogue::Flags<RoomFlag>;

struct room {
	coord r_pos;			/* Upper left corner */
	coord r_max;			/* Size of room */
	coord r_gold;			/* Where the gold is */
	int r_goldval;			/* How much the gold is worth */
	RoomFlags r_flags;		/* Info about the room */
	int r_nexits;			/* Number of exits */
	coord r_exit[12];			/* Where the exits are */
};

/*
 * Structure describing a fighting being
 */
struct stats {
	str_t s_str;			/* Strength */
	long s_exp;				/* Experience */
	int s_lvl;			/* Level of mastery */
	int s_arm;			/* Armor class */
	int s_hpt;			/* Hit points */
	const char *s_dmg;			/* String describing damage done */
	int s_maxhp;			/* Max hit points */
};

/*
 * The legacy union thing is split into a creature (monster or player) and an
 * item. o_charges and o_goldval are other names for o_ac.
 */
#include "entities/Item.hpp"
#include "entities/Creature.hpp"

using rogue::Creature;
using rogue::Item;
using rogue::List;
using rogue::ItemKind;
using rogue::ItemFilter;
using rogue::glyph_of;
using rogue::kind_of_glyph;
using rogue::CreatureFlags;
using rogue::ItemFlags;

/*
 * Various flag bits: rogue::ItemFlag and rogue::CreatureFlag, in Flags sets
 */
inline constexpr rogue::ItemFlag ISCURSED = rogue::ItemFlag::Cursed;
inline constexpr rogue::ItemFlag ISKNOW = rogue::ItemFlag::Known;
inline constexpr rogue::ItemFlag DIDFLASH = rogue::ItemFlag::DidFlash;
inline constexpr rogue::ItemFlag ISEGO = rogue::ItemFlag::Ego;
inline constexpr rogue::ItemFlag ISMISL = rogue::ItemFlag::Missile;
inline constexpr rogue::ItemFlag ISMANY = rogue::ItemFlag::Many;
inline constexpr rogue::ItemFlag ISREVEAL = rogue::ItemFlag::Revealed;
inline constexpr rogue::CreatureFlag ISBLIND = rogue::CreatureFlag::Blind;
inline constexpr rogue::CreatureFlag SEEMONST = rogue::CreatureFlag::SeeMonst;
inline constexpr rogue::CreatureFlag ISRUN = rogue::CreatureFlag::Running;
inline constexpr rogue::CreatureFlag ISFOUND = rogue::CreatureFlag::Found;
inline constexpr rogue::CreatureFlag ISINVIS = rogue::CreatureFlag::Invisible;
inline constexpr rogue::CreatureFlag ISMEAN = rogue::CreatureFlag::Mean;
inline constexpr rogue::CreatureFlag ISGREED = rogue::CreatureFlag::Greedy;
inline constexpr rogue::CreatureFlag ISHELD = rogue::CreatureFlag::Held;
inline constexpr rogue::CreatureFlag ISHUH = rogue::CreatureFlag::Confused;
inline constexpr rogue::CreatureFlag ISREGEN = rogue::CreatureFlag::Regen;
inline constexpr rogue::CreatureFlag CANHUH = rogue::CreatureFlag::CanConfuse;
inline constexpr rogue::CreatureFlag CANSEE = rogue::CreatureFlag::SeeInvisible;
inline constexpr rogue::CreatureFlag ISCANC = rogue::CreatureFlag::Cancelled;
inline constexpr rogue::CreatureFlag ISSLOW = rogue::CreatureFlag::Slow;
inline constexpr rogue::CreatureFlag ISHASTE = rogue::CreatureFlag::Hasted;
inline constexpr rogue::CreatureFlag ISFLY = rogue::CreatureFlag::Flying;

#define o_charges	o_ac
#define o_goldval	o_ac

/*
 * Array containing information on all the various types of monsters
 */
struct monster {
	const char *m_name;			/* What to call the monster */
	int m_carry;			/* Probability of carrying something */
	CreatureFlags m_flags;		/* Things about the monster */
	struct stats m_stats;		/* Initial stats */
};

// The tables each game copies into game().items (extern.cpp)
extern const struct magic_item s_magic_base[], p_magic_base[], r_magic_base[],
				ws_magic_base[], things_base[];

#include "game/Game.hpp"
#include "items/ItemCatalog.hpp"
#include "items/Identification.hpp"
#include "items/Inventory.hpp"
#include "items/effects/Potion.hpp"
#include "items/effects/Scroll.hpp"
#include "items/effects/Wand.hpp"
#include "items/effects/Ring.hpp"
#include "items/effects/Armor.hpp"
#include "items/effects/Weapon.hpp"
#include "rules/Daemons.hpp"
#include "rules/Combat.hpp"
#include "entities/MonsterCatalog.hpp"
#include "entities/MonsterAI.hpp"
#include "world/Rooms.hpp"
#include "world/Maze.hpp"
#include "world/Passages.hpp"
#include "world/LevelGenerator.hpp"
#include "game/CommandDispatcher.hpp"

using rogue::items::new_thing;
using rogue::items::inv_name;
using rogue::items::discovered;
using rogue::items::add_line;
using rogue::items::end_line;
using rogue::items::add_pack;
using rogue::items::pick_up;
using rogue::items::get_item;
using rogue::items::inventory;
using rogue::items::pack_char;
using rogue::items::money;
using rogue::items::drop;
using rogue::items::can_drop;
using rogue::items::effects::quaff;
using rogue::items::effects::invis_on;
using rogue::items::effects::turn_see;
using rogue::items::effects::th_effect;
using rogue::items::effects::read_scroll;
using rogue::items::effects::fix_stick;
using rogue::items::effects::do_zap;
using rogue::items::effects::drain;
using rogue::items::effects::fire_bolt;
using rogue::items::effects::charge_str;
using rogue::items::effects::ring_on;
using rogue::items::effects::ring_off;
using rogue::items::effects::ring_eat;
using rogue::items::effects::ring_num;
using rogue::items::effects::wear;
using rogue::items::effects::take_off;
using rogue::items::effects::waste_time;
using rogue::items::effects::missile;
using rogue::items::effects::do_motion;
using rogue::items::effects::fall;
using rogue::items::effects::init_weapon;
using rogue::items::effects::hit_monster;
using rogue::items::effects::num;
using rogue::items::effects::wield;
using rogue::items::effects::tick_pause;
using rogue::rules::Event;
using rogue::rules::start_daemon;
using rogue::rules::fuse;
using rogue::rules::lengthen;
using rogue::rules::extinguish;
using rogue::rules::do_daemons;
using rogue::rules::do_fuses;
using rogue::rules::doctor;
using rogue::rules::swander;
using rogue::rules::rollwand;
using rogue::rules::unconfuse;
using rogue::rules::unsee;
using rogue::rules::sight;
using rogue::rules::nohaste;
using rogue::rules::stomach;
using rogue::rules::fight;
using rogue::rules::attack;
using rogue::rules::swing;
using rogue::rules::check_level;
using rogue::rules::save_throw;
using rogue::rules::save;
using rogue::rules::is_magic;
using rogue::rules::raise_level;
using rogue::rules::killed;
using rogue::entities::randmonster;
using rogue::entities::pick_mons;
using rogue::entities::new_monster;
using rogue::entities::f_restor;
using rogue::entities::wanderer;
using rogue::entities::give_pack;
using rogue::entities::wake_monster;
using rogue::entities::moat;
using rogue::entities::runners;
using rogue::entities::start_run;
using rogue::entities::see_monst;
using rogue::entities::find_dest;
using rogue::entities::slime_split;
using rogue::entities::plop_monster;
using rogue::world::roomin;
using rogue::world::diag_ok;
using rogue::world::cansee;
using rogue::world::rnd_pos;
using rogue::world::enter_room;
using rogue::world::leave_room;
using rogue::world::new_level;
using rogue::world::rnd_room;
using rogue::command;
using rogue::show_count;
using rogue::execcom;

/*
 * External variables
 * The state of a game is in game() (game/Game.hpp). What is left here are
 * fixed tables and strings (extern.cpp, init.cpp).
 */

extern char nullstr[];
extern const char *it, *you, *no_mem;

#ifdef WIZARD
bool wizard;
#endif

extern const char *a_names[], *he_man[], *intense, *w_names[];
// a std::format string for msg()
inline constexpr const char *flashmsg = "your {} gives off a flash{}";
extern struct h_list helpcoms[], helpobjs[];
extern int	a_chances[], a_class[];
extern struct monster	monsters[];

// the experience level table (init.cpp)
extern const long e_levels[20];

/*
 * Function types
 * mach_dep.cpp functions are declared in extern.h
 */

// init.cpp
void	init_player(void);
void	init_things(void);
void	init_colors(void);
void	init_names(void);
void	init_stones(void);
void	init_materials(void);
char	*getsyl(void);
char	rchr(const char *string);

// io.cpp
// msg(), addmsg() and ifterse() take std::format strings. An empty msg() clears the line.
void	show_msg(std::string_view text);
void	add_msg(std::string_view text);

template <class... Args>
void
msg(std::format_string<Args...> fmt, Args &&...args)
{
	show_msg(std::format(fmt, std::forward<Args>(args)...));
}

template <class... Args>
void
addmsg(std::format_string<Args...> fmt, Args &&...args)
{
	add_msg(std::format(fmt, std::forward<Args>(args)...));
}

template <class... Args>
void
ifterse(std::format_string<Args...> tfmt, std::format_string<Args...> fmt, Args &&...args)
{
	msg(game().options.expert ? tfmt : fmt, std::forward<Args>(args)...);
}

void	wait_msg(const char *msg);
void	endmsg(void);
void	more(const char *msg);
void	putmsg(char *msg);
void	status(void);
void	wait_for(unsigned char ch);
void	show_win(char *message);
void	str_attr(const char *str);
void	SIG2(void);
std::string	io_unctrl(unsigned char ch);
const char	*noterse(const char *str);

// list.cpp
Item	*new_item(void);
Creature	*new_creature(void);
int	discard(Item *item);
int	discard(Creature *item);

/*
 * Empties a list of creatures or items and gives them back to the pool
 */
template <class T>
void
list_free(rogue::List<T> &list)
{
	T *item;

	while ((item = list.first()) != NULL)
	{
	detach(list, item);
	discard(item);
	}
}

// playit.cpp
void	endit(void);
void	playit(char *sname);
void	quit(void);
void	leave(void);
// legacy wrappers around rogue::rng()
inline int	rnd(int range) { return rogue::rng().below(range); }
inline int	roll(int number, int sides) { return rogue::rng().roll(number, sides); }

// misc.cpp
void	look(bool wakeup);
void	eat(void);
void	chg_str(int amt);
void	add_str(str_t *sp, int amt);
void	aggravate(void);
void	call_it(bool know, char **guess);
void	help(struct h_list *helpscr);
void	search(void);
void	d_level(void);
void	u_level(void);
void	call(void);
void	do_macro(char *buf, int sz);
Item	*find_obj(int y, int x);
bool	add_haste(bool potion);
bool	is_current(Item *obj);
bool	get_dir(void);
bool	find_dir(unsigned char ch, coord *cp);
bool	step_ok(unsigned char ch);
bool	offmap(int y, int x);
const char	*tr_name(unsigned char type);
const char	*vowelstr(const char *str);
char	goodch(Item *obj);
int	sign(int nm);
unsigned char	winat(int y, int x);
int	spread(int nm);
int	DISTANCE(int y1, int x1, int y2, int x2);
int	INDEX(int y, int x);

// move.cpp
void	do_run(unsigned char ch);
void	do_move(int dy, int dx);
void	door_open(struct room *rp);
void	descend(const char *mesg);
void	rndmove(Creature *who, coord *newmv);

// rip.cpp
void	score(int amount, int flags, char monst);
void	death(char monst);
void	total_winner(void);
std::string	killname(unsigned char monst, bool doart);

// save.cpp
void	save_game(void);
void	restore(char *savefile);

// strings.cpp
bool	is_alpha(char ch);
bool	is_upper(char ch);
bool	is_lower(char ch);
bool	is_digit(char ch);
bool	is_space(char ch);
bool	is_print(char ch);
char	*stccpy(char *s1, char *s2, int count);
char	*stpblk(char *str);
char	*endblk(char *str);
void	lcase(char *str);

// wizard.cpp
void	whatis(void);
int	teleport(void);
#ifdef WIZARD
void	create_obj();
#endif //WIZARD
