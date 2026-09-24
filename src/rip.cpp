/*
 * File for the fun ends
 * Death or a total win
 *
 * rip.c	1.4 (A.I. Design)	12/14/84
 */

#include "rogue.h"

//@ moved from rogue.h
#define TOPSCORES	10
struct sc_ent {
	char sc_name[38];
	int sc_rank;
	int sc_gold;
	int sc_fate;
	int sc_level;
};

static FILE *file;

static void	get_scores(struct sc_ent *top10);
static void	put_scores(struct sc_ent *top10);
static void	pr_scores(int newrank, struct sc_ent *top10);
static int	add_scores(struct sc_ent *newscore, struct sc_ent *oldlist);

/*
 * score:
 *	Figure score and post it.
 */
/* VARARGS2 */
void
score(int amount, int flags, char monst)
{
#ifndef WIZARD
	struct sc_ent his_score, top_ten[TOPSCORES];
	int rank=0;
	char response = ' ';


	display().open_page();  //@ stops the clock, as is_saved did

	if (amount || flags || monst)
	{
		wait_msg("see rankings");
	}
	while ((file = fopen(game().options.score_file, "r")) == NULL)
	{
		display().write("\n");
		if (game().noscore || (amount == 0))
			return;
		str_attr("No scorefile: %Create %Retry %Abort");
reread:
		switch(response = readchar())
		{
		case 'c':
		case 'C':
			fclose(fopen(game().options.score_file, "w"));
			break;
		case 'r':
		case 'R':
			break;
		case 'a':
		case 'A':
			return;
		default:
			goto reread;
		}
	}
	display().write("\n");
	get_scores(top_ten);

	if (game().noscore != TRUE)
	{
		strcpy(his_score.sc_name,game().options.name);
		his_score.sc_gold = amount;
		his_score.sc_fate = flags ? flags : monst;
		his_score.sc_level = max_level;
		his_score.sc_rank  = pstats.s_lvl;
		rank = add_scores(&his_score, top_ten);
	}
	fclose(file);
	if (rank > 0) {
		if ((file = fopen(game().options.score_file, "w")) != NULL) {
			put_scores(top_ten);
			fclose(file);
		}
	}
	pr_scores(rank, top_ten);
	wait_msg("exit");
	display().write("\n");
#endif //WIZARD
}

#ifndef WIZARD
static
void
get_scores(struct sc_ent *top10)
{
	int i, retcode = 1;

	for(i=0; i<TOPSCORES; i++,top10++) {
		if (retcode > 0)
			retcode = fread(top10, sizeof(struct sc_ent), 1, file);
		if (retcode <= 0)
			top10->sc_gold = 0;
	}
}

static
void
put_scores(struct sc_ent *top10)
{
	int i;

	for (i=0;(i<TOPSCORES) && top10->sc_gold;i++,top10++)
	{
		if (fwrite(top10, sizeof(struct sc_ent), 1, file) <= 0)
			return;
	}
}

static
void
pr_scores(int newrank, struct sc_ent *top10)
{
	int i, n;
	char dthstr[30];
	char texts[TOPSCORES][MAXSTR];
	rogue::ui::ScoreLine lines[TOPSCORES];
	const char *altmsg;

	for (i=0,n=0;i<TOPSCORES;i++,top10++)
	{
		char *text = texts[n];

		altmsg = NULL;
		if (top10->sc_gold <=0 )
			break;
		if (top10->sc_level >= 26)  //@ There is AMULETLEVEL, you know?
			altmsg = " Honored by the Guild";

		if (is_alpha(top10->sc_fate))
		{
			sprintf(dthstr," killed by %s",
				killname((0xff & top10->sc_fate), TRUE));
		}
		else
		{
			switch(top10->sc_fate)
			{
				case 2:
					altmsg = " A total winner!";
					break;
				case 1:
					strcpy(dthstr," quit");
					break;
				default:
					strcpy(dthstr," wierded out");
					break;
			}
		}
		text[0] = '\0';
		if ((signed)(strlen(top10->sc_name) + 10 +
			strlen(he_man[top10->sc_rank-1])) < COLS)
		{
			if (top10->sc_rank > 1 && (strlen(top10->sc_name)))
				sprintf(text, " \"%s\"",he_man[top10->sc_rank - 1]);
		}
		if (altmsg == NULL)
			sprintf(text + strlen(text), "%s on level %d",dthstr,top10->sc_level);
		else
			strcat(text, altmsg);
		lines[n].gold = top10->sc_gold;
		lines[n].name = top10->sc_name;
		lines[n].text = text;
		n++;
	}
	display().draw_scores(std::span(lines, n), newrank - 1);
}

static
int
add_scores(struct sc_ent *newscore, struct sc_ent *oldlist)
{
	struct sc_ent *sentry, *insert;
	int retcode = TOPSCORES+1;

	for(sentry=&oldlist[TOPSCORES-1];sentry>=oldlist;sentry--) {
		if ((unsigned)newscore->sc_gold > (unsigned)sentry->sc_gold) {
			insert = sentry;
			retcode--;
			if ((insert < &oldlist[TOPSCORES-1]) && sentry->sc_gold)
				sentry[1] = *sentry;
		}
		else
			break;
	}
	if (retcode == 11)
		return 0;
	*insert = *newscore;
	return retcode;
}
#endif //WIZARD

/*
 * death:
 *	Do something really fun when he dies
 */
void
death(char monst)
{
	int year;

	purse -= purse / 10;

	display().curtain_down();
	//@ killname() leaves the death reason in prbuf
	killname(monst, TRUE);
	year = md_localtime()->year;
	display().draw_tombstone(game().options.name, prbuf, purse, year);
	display().curtain_up();
	display().write_at(LINES-1, 0, "");
	score(purse, 0, monst);
	md_exit(EXIT_SUCCESS);
}

/*
 * total_winner:
 *	Code for a winner
 */
void
total_winner(void)
{
	THING *obj;
	int worth = 0;
	byte c;
	int oldpurse;
	char buf[132];  //@ as printw() had

	display().draw_winner(game().options.terse);
	wait_for(' ');
	display().clear_page();
	display().write_at(0, 0, "   Worth  Item");
	oldpurse = purse;
	for (c = 'a', obj = pack; obj != NULL; c++, obj = next(obj))
	{
	switch (obj->o_type)
	{
		when FOOD:
			worth = 2 * obj->o_count;
		when WEAPON:
			switch (obj->o_which)
			{
				when MACE: worth = 8;
				when SWORD: worth = 15;
				when CROSSBOW: worth = 30;
				when ARROW: worth = 1;
				when DAGGER: worth = 2;
				when TWOSWORD: worth = 75;
				when DART: worth = 1;
				when BOW: worth = 15;
				when BOLT: worth = 1;
				when SPEAR: worth = 5;
				break;
			}
			worth *= 3 * (obj->o_hplus + obj->o_dplus) + obj->o_count;
			obj->o_flags |= ISKNOW;
		when ARMOR:
			switch (obj->o_which)
			{
				when LEATHER: worth = 20;
				when RING_MAIL: worth = 25;
				when STUDDED_LEATHER: worth = 20;
				when SCALE_MAIL: worth = 30;
				when CHAIN_MAIL: worth = 75;
				when SPLINT_MAIL: worth = 80;
				when BANDED_MAIL: worth = 90;
				when PLATE_MAIL: worth = 150;
				break;
			}
			worth += (9 - obj->o_ac) * 100;
			worth += (10 * (a_class[obj->o_which] - obj->o_ac));
			obj->o_flags |= ISKNOW;
		when SCROLL:
			worth = s_magic[obj->o_which].mi_worth;
			worth *= obj->o_count;
			if (!s_know[obj->o_which])
				worth /= 2;
			s_know[obj->o_which] = TRUE;
		when POTION:
			worth = p_magic[obj->o_which].mi_worth;
			worth *= obj->o_count;
			if (!p_know[obj->o_which])
				worth /= 2;
			p_know[obj->o_which] = TRUE;
		when RING:
			worth = r_magic[obj->o_which].mi_worth;
			if (obj->o_which == R_ADDSTR || obj->o_which == R_ADDDAM ||
				obj->o_which == R_PROTECT || obj->o_which == R_ADDHIT)
			{
				if (obj->o_ac > 0)
					worth += obj->o_ac * 100;
				else
					worth = 10;
			}
			if (!(obj->o_flags & ISKNOW))
				worth /= 2;
			obj->o_flags |= ISKNOW;
			r_know[obj->o_which] = TRUE;
		when STICK:
			worth = ws_magic[obj->o_which].mi_worth;
			worth += 20 * obj->o_charges;
			if (!(obj->o_flags & ISKNOW))
				worth /= 2;
			obj->o_flags |= ISKNOW;
			ws_know[obj->o_which] = TRUE;
			when AMULET:
			worth = 1000;
			break;
	}
	if (worth < 0)
		worth = 0;
	snprintf(buf, sizeof buf, "%c) %5d  %s", c, worth, inv_name(obj, FALSE));
	display().write_at(c - 'a' + 1, 0, buf);
	purse += worth;
	}
	snprintf(buf, sizeof buf, "   %5u  Gold Pieces          ", oldpurse);
	display().write_at(c - 'a' + 1, 0, buf);
	score(purse, 2, 0);
	md_exit(EXIT_SUCCESS);
}

/*
 * killname:
 *	Convert a code to a monster name
 */
char *
killname(byte monst, bool doart)
{
	const char *sp;
	bool article;

	sp = prbuf;
	article = TRUE;
	switch (monst)
	{
	when 'a':
		sp = "arrow";
	when 'b':
		sp = "bolt";
	when 'd':
		sp = "dart";
	when 's':
		sp = "starvation";
		article = FALSE;
	when 'f':
		sp = "fall";
	otherwise:
		if (ismonster(monst))
			sp = monsters[monst-'A'].m_name;
		else
		{
			sp = "God";
			article = FALSE;
		}
	}
	if (doart && article)
	sprintf(prbuf, "a%s ", vowelstr(sp));
	else
	prbuf[0] = '\0';
	strcat(prbuf, sp);
	return prbuf;
}

