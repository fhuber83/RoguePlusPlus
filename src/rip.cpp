/*
 * File for the fun ends
 * Death or a total win
 *
 * rip.c	1.4 (A.I. Design)	12/14/84
 */

#include <vector>

#include "persistence/HighScores.hpp"
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

static bool	get_scores(struct sc_ent *top10, bool *legacy);
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
	bool legacy = false;
	bool readable = get_scores(top_ten, &legacy);

	if (game().noscore != TRUE)
	{
		strcpy(his_score.sc_name,game().options.name);
		his_score.sc_gold = amount;
		his_score.sc_fate = flags ? flags : monst;
		his_score.sc_level = game().player.max_level;
		his_score.sc_rank  = pstats.s_lvl;
		rank = add_scores(&his_score, top_ten);
	}
	fclose(file);
	//@ an unreadable file is left alone; an old binary one is rewritten as JSON
	if (readable && (rank > 0 || legacy))
		put_scores(top_ten);
	pr_scores(rank, top_ten);
	if (!readable)
		display().write("The score file can't be read, so this score is not kept.\n");
	wait_msg("exit");
	display().write("\n");
#endif //WIZARD
}

#ifndef WIZARD
/*@
 * get_scores:
 *	Fill top10 from the score file (persistence/HighScores); the entries
 *	after the last have no gold. Returns false, with an empty list, if the
 *	file can't be read. *legacy is set for a file in the original format.
 */
static
bool
get_scores(struct sc_ent *top10, bool *legacy)
{
	for (int i = 0; i < TOPSCORES; i++)
		top10[i] = sc_ent{};
	auto list = rogue::persistence::load_scores(game().options.score_file);
	if (!list)
		return false;
	*legacy = list->format == rogue::persistence::ScoresFormat::Legacy;
	int i = 0;
	for (const rogue::persistence::ScoreEntry &e : list->entries) {
		snprintf(top10[i].sc_name, sizeof top10[i].sc_name, "%s", e.name.c_str());
		top10[i].sc_gold = e.gold;
		top10[i].sc_level = e.depth;
		top10[i].sc_rank = e.experience;
		top10[i].sc_fate = e.fate;
		i++;
	}
	return true;
}

/*@
 * put_scores:
 *	Write the entries with gold to the score file, with the cause of each
 *	fate in words.
 */
static
void
put_scores(struct sc_ent *top10)
{
	std::vector<rogue::persistence::ScoreEntry> entries;

	for (int i = 0; i < TOPSCORES && top10[i].sc_gold; i++)
	{
		const sc_ent &sc = top10[i];
		rogue::persistence::ScoreEntry e;
		e.name = sc.sc_name;
		e.gold = sc.sc_gold;
		e.depth = sc.sc_level;
		e.experience = sc.sc_rank;
		e.fate = sc.sc_fate;
		if (is_alpha(sc.sc_fate))
			e.cause = std::string("killed by ") + killname(0xff & sc.sc_fate, TRUE);
		else
			e.cause = sc.sc_fate == 2 ? "a total winner" : sc.sc_fate == 1 ? "quit" : "weirded out";
		entries.push_back(std::move(e));
	}
	rogue::persistence::save_scores(game().options.score_file, entries);
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

	game().player.purse -= game().player.purse / 10;

	display().curtain_down();
	//@ killname() leaves the death reason in prbuf
	killname(monst, TRUE);
	year = md_localtime()->year;
	display().draw_tombstone(game().options.name, prbuf, game().player.purse, year);
	display().curtain_up();
	display().write_at(LINES-1, 0, "");
	score(game().player.purse, 0, monst);
	md_exit(EXIT_SUCCESS);
}

/*
 * total_winner:
 *	Code for a winner
 */
void
total_winner(void)
{
	Item *obj;
	int worth = 0;
	byte c;
	int oldpurse;
	char buf[132];  //@ as printw() had
	rogue::Items &items = game().items;

	display().draw_winner(game().options.terse);
	wait_for(' ');
	display().clear_page();
	display().write_at(0, 0, "   Worth  Item");
	oldpurse = game().player.purse;
	for (c = 'a', obj = pack.first(); obj != NULL; c++, obj = pack.after(obj))
	{
	switch (obj->o_type)
	{
		when ItemKind::Food:
			worth = 2 * obj->o_count;
		when ItemKind::Weapon:
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
			obj->o_flags.set(ISKNOW);
		when ItemKind::Armor:
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
			obj->o_flags.set(ISKNOW);
		when ItemKind::Scroll:
			worth = items.s_magic[obj->o_which].mi_worth;
			worth *= obj->o_count;
			if (!items.s_know[obj->o_which])
				worth /= 2;
			items.s_know[obj->o_which] = TRUE;
		when ItemKind::Potion:
			worth = items.p_magic[obj->o_which].mi_worth;
			worth *= obj->o_count;
			if (!items.p_know[obj->o_which])
				worth /= 2;
			items.p_know[obj->o_which] = TRUE;
		when ItemKind::Ring:
			worth = items.r_magic[obj->o_which].mi_worth;
			if (obj->o_which == R_ADDSTR || obj->o_which == R_ADDDAM ||
				obj->o_which == R_PROTECT || obj->o_which == R_ADDHIT)
			{
				if (obj->o_ac > 0)
					worth += obj->o_ac * 100;
				else
					worth = 10;
			}
			if (!obj->o_flags.test(ISKNOW))
				worth /= 2;
			obj->o_flags.set(ISKNOW);
			items.r_know[obj->o_which] = TRUE;
		when ItemKind::Stick:
			worth = items.ws_magic[obj->o_which].mi_worth;
			worth += 20 * obj->o_charges;
			if (!obj->o_flags.test(ISKNOW))
				worth /= 2;
			obj->o_flags.set(ISKNOW);
			items.ws_know[obj->o_which] = TRUE;
			when ItemKind::Amulet:
			worth = 1000;
			break;
	otherwise:	//@ the other kinds of item: nothing
		break;
	}
	if (worth < 0)
		worth = 0;
	snprintf(buf, sizeof buf, "%c) %5d  %s", c, worth, inv_name(obj, FALSE));
	display().write_at(c - 'a' + 1, 0, buf);
	game().player.purse += worth;
	}
	snprintf(buf, sizeof buf, "   %5u  Gold Pieces          ", oldpurse);
	display().write_at(c - 'a' + 1, 0, buf);
	score(game().player.purse, 2, 0);
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

