/*
 * Routines dealing specifically with rings
 *
 * rings.c		1.4 (AI Design)		12/13/84
 */

#include "rogue.h"

static int	gethand(void);

/*
 * ring_on:
 *	Put a ring on a hand
 */
void
ring_on()
{
	Item *obj;
	int ring = -1;
	rogue::Player &player = game().player;

	if ((obj = get_item("put on", ItemKind::Ring)) == NULL)
		goto no_ring;
	/*
	 * Make certain that it is somethings that we want to wear
	 */
	if (obj->o_type != ItemKind::Ring) {
		msg("you can't put that on your finger");
		goto no_ring;
	}

	/*
	 * find out which hand to put it on
	 */
	if (is_current(obj))
		goto no_ring;

	if (player.rings[LEFT] == NULL)
		ring = LEFT;
	if (player.rings[RIGHT] == NULL)
		ring = RIGHT;
	if (player.rings[LEFT] == NULL && player.rings[RIGHT] == NULL)
		if ((ring = gethand()) < 0)
			goto no_ring;
	if (ring < 0) {
		msg("you already have a ring on each hand");
		goto no_ring;
	}
	player.rings[ring] = obj;

	/*
	 * Calculate the effect it has on the poor guy.
	 */
	switch (obj->o_which) {
	case R_ADDSTR:
		chg_str(obj->o_ac);
		break;
	case R_SEEINVIS:
		invis_on();
		break;
	case R_AGGR:
		aggravate();
		break;
	}

	msg("%swearing %s (%c)", noterse("you are now "),
		inv_name(obj, TRUE), pack_char(obj));
	return ;

no_ring:
	game().turn.after = FALSE;
	return;
}

/*
 * ring_off:
 *	Take off a ring
 */
void
ring_off(void)
{
	int ring;
	Item *obj;
	char packchar;
	rogue::Player &player = game().player;

	if (player.rings[LEFT] == NULL && player.rings[RIGHT] == NULL) {
		msg("you aren't wearing any rings");
		game().turn.after = FALSE;
		return;
	} else if (player.rings[LEFT] == NULL)
		ring = RIGHT;
	else if (player.rings[RIGHT] == NULL)
		ring = LEFT;
	else
		if ((ring = gethand()) < 0)
			return;
	game().message.end = 0;
	obj = player.rings[ring];
	if (obj == NULL) {
		msg("not wearing such a ring");
		game().turn.after = FALSE;
		return;
	}
	packchar = pack_char(obj);
	if (can_drop(obj))
		msg("was wearing %s(%c)", inv_name(obj, TRUE), packchar);
}

/*
 * gethand:
 *	Which hand is the hero interested in?
 */
static
int
gethand(void)
{
	int c;

	for (;;) {
		msg("left hand or right hand? ");
		if ((c = readchar()) == ESCAPE)  {
			game().turn.after = FALSE;
			return -1;
		}
		game().message.end = 0;
		if (c == 'l' || c == 'L')
			return LEFT;
		else if (c == 'r' || c == 'R')
			return RIGHT;
		msg("please type L or R");
	}
	return -1;
}

/*
 * ring_eat:
 *	How much food does this ring use up?
 */
int
ring_eat(int hand)
{
	if (game().player.rings[hand] == NULL)
		return 0;
	switch (game().player.rings[hand]->o_which) {
	case R_REGEN:
		return 2;
	case R_SUSTSTR:
	case R_SUSTARM:
	case R_PROTECT:
	case R_ADDSTR:
	case R_STEALTH:
		return 1;
	case R_SEARCH:
		return(rnd(5)==0);
	case R_ADDHIT:
	case R_ADDDAM:
		return (rnd(3) == 0);
	case R_DIGEST:
		return -rnd(2);
	case R_SEEINVIS:
		return (rnd(5) == 0);
	default:
		return 0;
	}
}

/*
 * ring_num:
 *	Print ring bonuses
 */
const char *
ring_num(Item *obj)
{
	if (!obj->o_flags.test(ISKNOW))
		return "";
	switch (obj->o_which) {
	when R_PROTECT:
	case R_ADDSTR:
	case R_ADDDAM:
	case R_ADDHIT:
		ring_buf[0] = ' ';
		strcpy(&ring_buf[1], num(obj->o_ac, 0, RING));
	otherwise:
		return "";
	}
	return ring_buf;
}
