#include "rogue.h"

namespace rogue::items::effects {

const char *laugh = "you hear maniacal laughter%s.";
const char *in_dist = " in the distance";
/*
 * read_scroll:
 *	Read a scroll from the pack and do the appropriate thing
 */
void
read_scroll()
{
	Item *obj;
	int y, x;
	byte ch;
	Item *op;
	Creature *mo;
	int index;
	bool discardit = FALSE;
	rogue::Player &player = game().player;
	rogue::Level &level = game().level;
	rogue::Items &items = game().items;

	obj = get_item("read", ItemKind::Scroll);
	if (obj == NULL)
		return;
	if (obj->o_type != ItemKind::Scroll){
		msg("there is nothing on it to read");
		return;
	}
	ifterse0("the scroll vanishes","as you read the scroll, it vanishes");
	/*
	 * Calculate the effect it has on the poor guy.
	 */
	if (obj == player.weapon)
		player.weapon = NULL;
	switch (obj->o_which){
	when S_CONFUSE:
		/*
		 * Scroll of monster confusion.  Give him that power.
		 */
		player.body.t_flags.set(CANHUH);
		msg("your hands begin to glow red");
	when S_ARMOR:
		if (player.armor != NULL) {
			player.armor->o_ac--;
			player.armor->o_flags.unset(ISCURSED);
			ifterse0("your armor glows faintly",
				"your armor glows faintly for a moment");
		}
	when S_HOLD:
		/*
		 * Hold monster scroll.  Stop all monsters within two spaces
		 * from chasing after the hero.
		 */

		for (x = hero.x - 3; x <= hero.x + 3; x++)
			if (x >= 0 && x < COLS)
				for (y = hero.y - 3; y <= hero.y + 3; y++)
					if ((y > 0 && y < maxrow) && ((mo=moat(y, x)) != NULL)) {
						mo->t_flags.unset(ISRUN);
						mo->t_flags.set(ISHELD);
					}
	when S_SLEEP:
		/*
		 * Scroll which makes you fall asleep
		 */
		items.s_know[S_SLEEP] = TRUE;
		player.no_command += rnd(SLEEPTIME) + 4;
		player.body.t_flags.unset(ISRUN);
		msg("you fall asleep");
	when S_CREATE:
		{
		coord mp;

		if (plop_monster(hero.y, hero.x, &mp) && (mo=new_creature()) != NULL)
			new_monster(mo, randmonster(FALSE), &mp);
		else
			ifterse0("you hear a faint cry of anguish",
				"you hear a faint cry of anguish in the distance");
		}
	when S_IDENT:
		/*
		 * Identify, let the rogue figure something out
		 */
		items.s_know[S_IDENT] = TRUE;
		msg("this scroll is an identify scroll");
		if (! strcmp(game().options.menu,"on") || !strcmp(game().options.menu,"sel"))
			more(" More ");
		whatis();
	when S_MAP:
		/*
		 * Scroll of magic mapping.
		 */
		items.s_know[S_MAP] = TRUE;
		msg("oh, now this scroll has a map on it");
		/*
		 * Take all the things we want to keep hidden out of the window
		 */
		for (y = 1; y < maxrow; y++)
			for (x = 0; x < COLS; x++) {
				index = INDEX(y, x);
				switch (ch = level.map[index])
				{
				case VWALL:
				case HWALL:
				case ULWALL:
				case URWALL:
				case LLWALL:
				case LRWALL:
					if (!(level.flags[index] & F_REAL)) {
						ch = level.map[index] = DOOR;
						level.flags[index] &= ~F_REAL;
					}
					/* fallthrough */
				case DOOR:
				case PASSAGE:
				case STAIRS:
					if ((mo = moat(y, x)) != NULL)
						if (mo->t_oldch == ' ')
							mo->t_oldch = ch;
					break;
				default:
					ch = ' ';
				}
				if (ch != ' ')
					display().draw_tile({x, y}, ch,
							(ch == DOOR && display().tile_at({x, y}) != DOOR)
								? TileStyle::Inverse : TileStyle::Normal);
			}
	when S_GFIND:
		/*
		 * Scroll of food detection
		 */
		ch = FALSE;
		for (op = level.objects.first(); op != NULL; op = level.objects.after(op)) {
			if (op->o_type == ItemKind::Food) {
				ch = TRUE;
				display().draw_tile(op->o_pos, FOOD, TileStyle::Inverse);
			} else /* as a bonus this will detect amulets as well */
			if (op->o_type == ItemKind::Amulet) {
				ch = TRUE;
				display().draw_tile(op->o_pos, AMULET, TileStyle::Inverse);
			}
		}
		if (ch) {
			items.s_know[S_GFIND] = TRUE;
			msg("your nose tingles as you sense food");
		} else
			ifterse0("you hear a growling noise close by","you hear a growling noise very close to you");
	when S_TELEP:
		/*
		 * Scroll of teleportation:
		 * Make him dissapear and reappear
		 */
		{
		struct room *cur_room;

		cur_room = proom;
		teleport();
		if (cur_room != proom)
			items.s_know[S_TELEP] = TRUE;
		}
	when S_ENCH:
		if (player.weapon == NULL || player.weapon->o_type != ItemKind::Weapon)
		msg("you feel a strange sense of loss");
		else
		{
		player.weapon->o_flags.unset(ISCURSED);
		if (rnd(2) == 0)
			player.weapon->o_hplus++;
		else
			player.weapon->o_dplus++;
		ifterse1("your %s glows blue","your %s glows blue for a moment", w_names[player.weapon->o_which]);
		}
	when S_SCARE:
		/*
		 * Reading it is a mistake and produces laughter at the
		 * poor rogue's boo boo.
		 */
			msg(laugh, game().options.brief() ? "" : in_dist);
	when S_REMOVE:
		if (player.armor != NULL)
			player.armor->o_flags.unset(ISCURSED);
		if (player.weapon != NULL)
			player.weapon->o_flags.unset(ISCURSED);
		if (player.rings[LEFT] != NULL)
			player.rings[LEFT]->o_flags.unset(ISCURSED);
		if (player.rings[RIGHT] != NULL)
			player.rings[RIGHT]->o_flags.unset(ISCURSED);
		ifterse0("somebody is watching over you","you feel as if somebody is watching over you");
	when S_AGGR:
		/*
		 * This scroll aggravates all the monsters on the current
		 * level and sets them running towards the hero
		 */
		aggravate();
		ifterse("you hear a humming noise",
					"you hear a high pitched humming noise");
	when S_NOP:
		msg("this scroll seems to be blank");
	when S_VORPAL:
		/*
		 * Extra Vorpal Enchant Weapon
		 *     Give weapon +1,+1
		 *     Is extremely vorpal against one certain type of monster
		 *     Against this type (o_enemy) the weapon gets:
		 *		+4,+4
		 *		The ability to zap one such monster into oblivion
		 *
		 *     Some of these are cursed and if the rogue misses her saving
		 *     throw she will be forced to attack monsters of this type
		 *     whenever she sees one (not yet implemented)
		 *
		 * If he doesn't have a weapon I get to chortle again!
		 */
		if (player.weapon == NULL || player.weapon->o_type != ItemKind::Weapon)
			msg(laugh, game().options.brief() ? "" : in_dist);
		else {
			/*
			 * You aren't allowed to doubly vorpalize a weapon.
			 */
			if (player.weapon->o_enemy != 0) {
				msg("your %s vanishes in a puff of smoke",
				w_names[player.weapon->o_which]);
				detach(pack, player.weapon);
				discard(player.weapon);
				player.weapon = NULL;
			} else {
				player.weapon->o_enemy = pick_mons();
				player.weapon->o_hplus++;
				player.weapon->o_dplus++;
				player.weapon->o_charges = 1;
				msg(flashmsg, w_names[player.weapon->o_which],
					game().options.brief() ? "" : intense);

				/*
				 * Sometimes this is a mixed blessing ...
					if (rnd(20) == 0) {
						cur_weapon->o_flags.set(ISCURSED);
						if (!save(VS_MAGIC)) {
							cur_weapon->o_flags.set(ISEGO|ISREVEAL);
							s_know[S_VORPAL] = TRUE;
							msg("you feel a sudden desire to kill %ss.",
							monsters[cur_weapon->o_enemy-'A'].m_name);
						}
					}
				 */
			}
		}
	otherwise:
		msg("what a puzzling scroll!");
		return;
	}
	look(TRUE);	/* put the result of the scroll on the screen */
	status();
	/*
	 * Get rid of the thing
	 */
	player.in_pack--;
	if (obj->o_count > 1)
	obj->o_count--;
	else
	{
	detach(pack, obj);
	discardit = TRUE;
	}
	call_it(items.s_know[obj->o_which], &items.s_guess[obj->o_which]);

	if (discardit)
	discard(obj);
}

}  // namespace rogue::items::effects
