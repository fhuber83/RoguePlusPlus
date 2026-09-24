/*
 * Rogue definitions and variable declarations
 *
 * rogue.h	1.4 (AI Design) 12/14/84
 */

/*@
 * Modern headers first: extern.h and this file define macros such as max(),
 * pack and when that would break standard library headers.
 */
#include <optional>

#include "core/Coord.hpp"
#include "core/Dice.hpp"
#include "core/Flags.hpp"
#include "core/Random.hpp"
#include "entities/List.hpp"
#include "ui/Display.hpp"
#include "ui/Input.hpp"

#include "extern.h"
#include "glyphs.h"

/*@
 * Screen size. Fixed at 80x25 (see rogue::ui::Screen); these used to be the
 * ncurses globals of the same name. Only game files see these; the curses
 * backend uses ncurses' own.
 */
const int LINES = MAXLINES;
const int COLS = MAXCOLS;
//@ Last line used for the map. Was a global, set in setup()
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

#define ifterse0 ifterse
#define ifterse1 ifterse
#define ifterse2 ifterse
#define ifterse3 ifterse
#define ifterse4 ifterse

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
#define BUFSIZE		128 /*@ moved from curses.h */

/*
 * All the fun defines
 */
#define shint		int		/* short integer (for very small #s) */
#define when		break;case
#define otherwise	break;default
#define until(expr)	while(!(expr))
#define hero		game().player.body.t_pos
#define pstats		game().player.body.t_stats
#define pack		game().player.body.t_pack
#define proom		game().player.body.t_room
#define max_hp		game().player.body.t_stats.s_maxhp
#define attach(a,b)	(a).push_front(b)
#define detach(a,b)	(a).remove(b)
#define free_list(a)	list_free(a)
#define max(a,b)	((a) > (b) ? (a) : (b))
#define on(thing,flag)	((thing).t_flags.test(flag))
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
/*@
 * And some new fun defines...
 */
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
 * @ this was unused in original. Now improved and put to good use
 */
struct h_list {
	byte h_chstr[6];  //@ either (ch) or (ch,sep,ch2) appended with ": "
	const char *h_desc;
};

/*
 * Coordinate data type
 */
using coord = rogue::Coord;  //@ see core/Coord.hpp

//@ Game output goes through the display, see ui/Display.hpp
using rogue::ui::display;
using rogue::ui::input;
using rogue::ui::TileStyle;

/*@
 * Data type for strength values and modifiers
 * That's very generous from Rogue devs to allow full 16-bits (uint in 1985)
 * for strength, considering normal play would not get even remotely close to 8
 */
typedef unsigned int str_t;

/*
 * Stuff about magic items
 */

struct magic_item {
	const char *mi_name;
	shint mi_prob;
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
	shint r_nexits;			/* Number of exits */
	coord r_exit[12];			/* Where the exits are */
};

/*
 * Structure describing a fighting being
 */
struct stats {
	str_t s_str;			/* Strength */
	long s_exp;				/* Experience */
	shint s_lvl;			/* Level of mastery */
	shint s_arm;			/* Armor class */
	shint s_hpt;			/* Hit points */
	const char *s_dmg;			/* String describing damage done */
	shint s_maxhp;			/* Max hit points */
};

/*@
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
 * Various flag bits
 * @ typed now: rogue::ItemFlag and rogue::CreatureFlag, in Flags sets
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
	shint m_carry;			/* Probability of carrying something */
	CreatureFlags m_flags;		/* Things about the monster */
	struct stats m_stats;		/* Initial stats */
};

//@ The tables each game copies into game().items (extern.cpp)
extern const struct magic_item s_magic_base[], p_magic_base[], r_magic_base[],
				ws_magic_base[], things_base[];

#include "game/Game.hpp"
#include "items/ItemCatalog.hpp"
#include "items/Identification.hpp"
#include "items/Inventory.hpp"

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

/*
 * External variables
 * @ The state of a game is in game() (game/Game.hpp). What is left here are
 * @ fixed tables and strings (extern.cpp) and scratch buffers (init.cpp).
 */

//@ nullstr should probably be used in misc and wizard instead of (size_t)NULL
extern char nullstr[];
extern const char *it, *you, *no_mem;

#ifdef WIZARD
bool wizard;
#endif

extern const char *a_names[], *flashmsg, *he_man[], *intense, *w_names[];
extern struct h_list helpcoms[], helpobjs[];
extern int	a_chances[], a_class[];
extern struct monster	monsters[];

/*@
 * Definition commented out:
 * extern bool askme, fight_flush, jump, passgo, slow_invent;
 * extern char *release;
 *
 * Not found:
 * extern bool in_shell;
 * extern char file_name[], home[], outbuf[];
 * extern int lastscore, is_me;
 */

//@ init.c: scratch buffers and the experience level table
extern char *tbuf, *prbuf;
extern long *e_levels;
extern char *ring_buf;
//@ extern char *_top, *_base;  //@ not found
/*@
 * Deprecated:
 * extern char *end_mem;
 */

/*
 * Function types
 *
 * @ curses.c has its own header
 * @ mach_dep.c functions are declared in extern.h
 */

//@ armor.c
void	wear(void);
void	take_off(void);
void	waste_time(void);

//@ chase.c
void	runners(void);
void	do_chase(Creature *th);
void	chase(Creature *tp, coord *ee);
void	start_run(coord *runner);
bool	see_monst(Creature *mp);
bool	diag_ok(coord *sp, coord *ep);
bool	cansee(int y, int x);
struct room	*roomin(coord *cp);
coord	*find_dest(Creature *tp);

//@ command.c
void	command(void);
void	show_count(void);
void	execcom(void);

//@ daemon.c
void	start_daemon(void (*func)());
void	do_daemons(void);
void	fuse(void (*func)(), int time);
void	lengthen(void (*func)(), int xtime);
void	extinguish(void (*func)());
void	do_fuses(void);

//@ daemons.c
void	doctor(void);
void	swander(void);
void	rollwand(void);
void	unconfuse(void);
void	unsee(void);
void	sight(void);
void	nohaste(void);
void	stomach(void);

//@ env.h
bool	setenv_from_file(const char *envfile);

//@ fight.c
bool	fight(coord *mp, char mn, Item *weap, bool thrown);
bool	swing(int at_lvl, int op_arm, int wplus);
bool	roll_em(Creature *thatt, Creature *thdef, Item *weap, bool hurl);
bool	save_throw(int which, Creature *tp);
bool	save(int which);
bool	is_magic(Item *obj);
void	attack(Creature *mp);
void	check_level(void);
void	hit(const char *er, const char *ee);
void	miss(const char *er, const char *ee);
void	raise_level(void);
void	thunk(Item *weap, const char *mname, const char *does, const char *did);
void	remove_monster(coord *mp, Creature *tp, bool waskill);
void	killed(Creature *tp, bool pr);
int	str_plus(str_t str);
int	add_dam(str_t str);

//@ init.c
void	init_player(void);
void	init_things(void);
void	init_colors(void);
void	init_names(void);
void	init_stones(void);
void	init_materials(void);
void	init_ds(void);
void	free_ds(void);
char	*getsyl(void);
char	rchr(const char *string);

//@ io.c
void	ifterse(const char *tfmt, const char *fmt, ...);
void	msg(const char *fmt, ...);
void	vmsg(const char *fmt, va_list argp);
void	addmsg(const char *fmt, ...);
void	doadd(const char *fmt, va_list argp);
void	wait_msg(const char *msg);
void	endmsg(void);
void	more(const char *msg);
void	putmsg(char *msg);
void	status(void);
void	wait_for(byte ch);
void	show_win(char *message);
void	str_attr(const char *str);
void	SIG2(void);
char	*io_unctrl(byte ch);
const char	*noterse(const char *str);

//@ list.c
Item	*new_item(void);
Creature	*new_creature(void);
int	discard(Item *item);
int	discard(Creature *item);

/*@
 * Empties a list of creatures or items and gives them back to the pool
 * (was _free_list)
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

//@ main.c
void	endit(void);
void	playit(char *sname);
void	quit(void);
void	leave(void);
//@ legacy wrappers around rogue::rng()
inline int	rnd(int range) { return rogue::rng().below(range); }
inline int	roll(int number, int sides) { return rogue::rng().roll(number, sides); }

//@ maze.c
void	draw_maze(struct room *rp);
void	new_frontier(int y, int x);
void	add_frnt(int y, int x);
void	con_frnt(void);
void	splat(int y, int x);
bool	maze_at(int y, int x);
bool	inrange(int y, int x);

//@ misc.c
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
bool	find_dir(byte ch, coord *cp);
bool	step_ok(byte ch);
bool	offmap(int y, int x);
const char	*tr_name(byte type);
const char	*vowelstr(const char *str);
char	goodch(Item *obj);
shint	sign(int nm);
byte	winat(int y, int x);
int	spread(int nm);
int	DISTANCE(int y1, int x1, int y2, int x2);
int	INDEX(int y, int x);

//@ monsters.c
char	randmonster(bool wander);
char	pick_mons(void);
void	new_monster(Creature *tp, byte type, coord *cp);
void	f_restor(void);
void	wanderer(void);
void	give_pack(Creature *tp);
Creature	*wake_monster(int y, int x);
Creature	*moat(int my, int mx);

//@ move.c
void	do_run(byte ch);
void	do_move(int dy, int dx);
void	door_open(struct room *rp);
void	descend(const char *mesg);
void	rndmove(Creature *who, coord *newmv);

//@ new_leve.c
void	new_level(void);
void	put_things(void);
int	rnd_room(void);

//@ passages.c
void	conn(int r1, int r2);
void	do_passages(void);
void	door(struct room *rm, coord *cp);
void	passnum(void);
void	numpass(int y, int x);
void	psplat(shint y, shint x);

//@ potions.c
void	quaff(void);
void	invis_on(void);
void	th_effect(Item *obj, Creature *tp);
bool	turn_see(bool turn_off);

//@ rings.c
void	ring_on(void);
void	ring_off(void);
const char	*ring_num(Item *obj);
int	ring_eat(int hand);

//@ rip.c
void	score(int amount, int flags, char monst);
void	death(char monst);
void	total_winner(void);
char	*killname(byte monst, bool doart);

//@ rooms.c
void	do_rooms(void);
void	draw_room(struct room *rp);
void	rnd_pos(struct room *rp, coord *cp);
void	enter_room(coord *cp);
void	leave_room(coord *cp);

//@ save.c
void	save_game(void);
void	restore(char *savefile);

//@ scrolls.c
void read_scroll(void);

//@ slime.c
void	slime_split(Creature *tp);
bool	plop_monster(int r, int c, coord *cp);

//@ sticks.c
void	fix_stick(Item *cur);
void	do_zap(void);
void	drain(void);
void	fire_bolt(coord *start, coord *dir, const char *name);
char	*charge_str(Item *obj);

//@ strings.c
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

//@ weapons.c
void	missile(int ydelta, int xdelta);
void	do_motion(Item *obj, int ydelta, int xdelta);
void	fall(Item *obj, bool pr);
void	init_weapon(Item *weap, byte type);
void	wield(void);
void	tick_pause(void);
char	*num(int n1, int n2, char type);
bool	hit_monster(int y, int x, Item *obj);

//@ wizard.c
void	whatis(void);
int	teleport(void);
#ifdef WIZARD
void	create_obj();
#endif //WIZARD


/*@ functions declared but not found
*/
