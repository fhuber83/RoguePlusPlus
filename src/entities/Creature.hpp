#pragma once

/*
 * A fighting being: a monster or the rogue himself.
 *
 * Was the _t half of the legacy union thing. Included by rogue.h after the
 * legacy types it uses (coord, struct stats, struct room); game files include
 * rogue.h.
 */

namespace rogue {

struct Item;

struct Creature {
	Creature *l_next, *l_prev;	/* Next pointer in link */
	coord t_pos;				/* Position */
	char t_turn;				/* If slowed, is it a turn to move */
	char t_type;				/* What it is */
	byte t_disguise;			/* What mimic looks like */
	byte t_oldch;				/* Character that was where it was */
	coord *t_dest;				/* Where it is running to */
	short t_flags;				/* State word */
	struct stats t_stats;		/* Physical description */
	struct room *t_room;		/* Current room for thing */
	Item *t_pack;				/* What the thing is carrying */
};

}  // namespace rogue
