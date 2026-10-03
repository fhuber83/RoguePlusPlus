/*
 * File for the fun ends
 * Death or a total win
 *
 * rip.c	1.4 (A.I. Design)	12/14/84
 */

#include "game/Endings.hpp"

#include <algorithm>
#include <bits/chrono.h>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <format>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/Ascii.hpp"
#include "core/Glyphs.hpp"
#include "core/Maybe.hpp"
#include "core/Text.hpp"
#include "entities/Item.hpp"
#include "entities/MonsterCatalog.hpp"
#include "game/Game.hpp"
#include "game/Keyboard.hpp"
#include "game/Messages.hpp"
#include "items/Identification.hpp"
#include "items/ItemCatalog.hpp"
#include "items/Kinds.hpp"
#include "persistence/HighScores.hpp"
#include "platform/Clock.hpp"
#include "platform/Session.hpp"
#include "rules/Experience.hpp"
#include "ui/Display.hpp"

namespace rogue {

using persistence::ScoreEntry;

namespace {

/*
 * get_scores:
 *	The scores in the score file (persistence/HighScores), richest first.
 *	nullopt if the file can't be read. *legacy is set for a file in the
 *	original format.
 */
std::optional<std::vector<ScoreEntry>>
get_scores(bool &legacy)
{
	auto list = persistence::load_scores(game().options.score_file);
	if (!list)
		return std::nullopt;
	legacy = list->format == persistence::ScoresFormat::Legacy;
	return std::move(list->entries);
}

/*
 * put_scores:
 *	Write the scores to the score file, with the cause of each fate in
 *	words.
 */
void
put_scores(std::vector<ScoreEntry> scores)
{
	for (ScoreEntry &e : scores)
	{
		if (is_alpha(e.fate))
			e.cause = std::string("killed by ") + killname(0xff & e.fate, true);
		else
			e.cause = e.fate == 2 ? "a total winner" : e.fate == 1 ? "quit" : "weirded out";
	}
	persistence::save_scores(game().options.score_file, scores);
}

void
pr_scores(int newrank, const std::vector<ScoreEntry> &top10)
{
	std::string dthstr;
	std::vector<std::string> texts;
	std::vector<ui::ScoreLine> lines;
	std::optional<std::string_view> altmsg;

	texts.reserve(top10.size());	// the lines point into them
	for (const ScoreEntry &sc : top10)
	{
		altmsg.reset();
		if (sc.gold <= 0)
			break;
		if (sc.depth >= 26)
			altmsg = " Honored by the Guild";

		if (is_alpha(sc.fate))
		{
			dthstr = " killed by " + killname((0xff & sc.fate), true);
		}
		else
		{
			switch(sc.fate)
			{
				case 2:
					altmsg = " A total winner!";
					break;
				case 1:
					dthstr = " quit";
					break;
				default:
					dthstr = " wierded out";
					break;
			}
		}
		std::string &text = texts.emplace_back();
		if (static_cast<int>(sc.name.size() + 10 + rules::he_man[sc.experience-1].size()) < MAXCOLS)
		{
			if (sc.experience > 1 && !sc.name.empty())
				text = std::format(" \"{}\"", rules::he_man[sc.experience - 1]);
		}
		if (!altmsg)
			text += std::format("{} on level {}", dthstr, sc.depth);
		else
			text += *altmsg;
		lines.push_back({.gold = sc.gold, .name = sc.name, .text = text});
	}
	ui::display().draw_scores(lines, newrank - 1);
}

}  // namespace

/*
 * add_score:
 *	Put a score in its place among the ten best: after those with as much
 *	gold or more, comparing as unsigned, as the original did.
 */
int
add_score(std::vector<ScoreEntry> &scores, const ScoreEntry &entry)
{
	// No gold is never more than an empty place, which has none
	if (entry.gold == 0)
		return 0;
	auto richer = [&](const ScoreEntry &e) {
		return static_cast<unsigned>(e.gold) >= static_cast<unsigned>(entry.gold);
	};
	std::size_t place = std::find_if_not(scores.begin(), scores.end(), richer) - scores.begin();
	if (place >= persistence::max_scores)
		return 0;
	scores.insert(scores.begin() + place, entry);
	if (scores.size() > persistence::max_scores)
		scores.pop_back();
	return static_cast<int>(place) + 1;
}

/*
 * score:
 *	Figure score and post it.
 */
void
score(int amount, int flags, char monst)
{
	int rank = 0;

	ui::display().open_page();  // stops the clock, as is_saved did

	if (amount || flags || monst)
	{
		wait_msg("see rankings");
	}
	while (!std::ifstream(game().options.score_file).is_open())
	{
		ui::display().write("\n");
		if (game().noscore || (amount == 0))
			return;
		str_attr("No scorefile: %Create %Retry %Abort");
		for (bool retry = false; !retry; )
		{
			switch (readchar())
			{
			case 'c':
			case 'C':
				std::ofstream(game().options.score_file);
				retry = true;
				break;
			case 'r':
			case 'R':
				retry = true;
				break;
			case 'a':
			case 'A':
				return;
			default:
				break;
			}
		}
	}
	ui::display().write("\n");
	bool legacy = false;
	std::optional<std::vector<ScoreEntry>> top_ten = get_scores(legacy);
	std::vector<ScoreEntry> unread;		// shown in its place: no scores

	if (game().noscore != true)
	{
		ScoreEntry his_score;
		his_score.name = game().options.name;
		his_score.gold = amount;
		his_score.fate = flags ? flags : monst;
		his_score.depth = game().player.max_level;
		his_score.experience = game().player.body.t_stats.s_lvl;
		rank = add_score(top_ten ? *top_ten : unread, his_score);
	}
	// an unreadable file is left alone; an old binary one is rewritten as JSON
	if (top_ten && (rank > 0 || legacy))
		put_scores(*top_ten);
	pr_scores(rank, top_ten ? *top_ten : unread);
	if (!top_ten)
		ui::display().write("The score file can't be read, so this score is not kept.\n");
	wait_msg("exit");
	ui::display().write("\n");
}

/*
 * death:
 *	Do something really fun when he dies
 */
void
death(char monst)
{
	int year;

	game().player.purse -= game().player.purse / 10;

	ui::display().curtain_down();
	year = static_cast<int>(std::chrono::year_month_day{
		std::chrono::floor<std::chrono::days>(rogue::platform::local_time(rogue::platform::now()))}.year());
	ui::display().draw_tombstone(game().options.name, killname(monst, true), game().player.purse, year);
	ui::display().curtain_up();
	ui::display().write_at(MAXLINES-1, 0, "");
	score(game().player.purse, 0, monst);
	platform::md_exit(EXIT_SUCCESS);
}

/*
 * total_winner:
 *	Code for a winner
 */
void
total_winner()
{
	Maybe<Item> obj;
	int worth = 0;
	unsigned char c;
	int oldpurse;
	rogue::Items &items = game().items;
	rogue::Player &player = game().player;

	ui::display().draw_winner(game().options.terse);
	wait_for(' ');
	ui::display().clear_page();
	ui::display().write_at(0, 0, "   Worth  Item");
	oldpurse = player.purse;
	for (c = 'a', obj = player.body.t_pack.first(); obj; c++, obj = player.body.t_pack.after(*obj))
	{
	switch (obj->o_type)
	{
		case ItemKind::Food:
			worth = 2 * obj->o_count;
			break;
		case ItemKind::Weapon:
			switch (obj->which<WeaponType>())
			{
				case WeaponType::Mace: worth = 8; break;
				case WeaponType::LongSword: worth = 15; break;
				case WeaponType::Crossbow: worth = 30; break;
				case WeaponType::Arrow: worth = 1; break;
				case WeaponType::Dagger: worth = 2; break;
				case WeaponType::TwoHandedSword: worth = 75; break;
				case WeaponType::Dart: worth = 1; break;
				case WeaponType::ShortBow: worth = 15; break;
				case WeaponType::CrossbowBolt: worth = 1; break;
				case WeaponType::Spear: worth = 5;
				break;
				default: break;
			}
			worth *= 3 * (obj->o_hplus + obj->o_dplus) + obj->o_count;
			obj->o_flags.set(ItemFlag::Known);
			break;
		case ItemKind::Armor:
			switch (obj->which<ArmorType>())
			{
				case ArmorType::Leather: worth = 20; break;
				case ArmorType::RingMail: worth = 25; break;
				case ArmorType::StuddedLeather: worth = 20; break;
				case ArmorType::ScaleMail: worth = 30; break;
				case ArmorType::ChainMail: worth = 75; break;
				case ArmorType::SplintMail: worth = 80; break;
				case ArmorType::BandedMail: worth = 90; break;
				case ArmorType::PlateMail: worth = 150;
				break;
			}
			worth += (9 - obj->o_ac) * 100;
			worth += (10 * (items::a_class[obj->which<ArmorType>()] - obj->o_ac));
			obj->o_flags.set(ItemFlag::Known);
			break;
		case ItemKind::Scroll:
			worth = items.s_magic[obj->which<Scroll>()].mi_worth;
			worth *= obj->o_count;
			if (!items.s_know[obj->which<Scroll>()])
				worth /= 2;
			items.s_know[obj->which<Scroll>()] = true;
			break;
		case ItemKind::Potion:
			worth = items.p_magic[obj->which<Potion>()].mi_worth;
			worth *= obj->o_count;
			if (!items.p_know[obj->which<Potion>()])
				worth /= 2;
			items.p_know[obj->which<Potion>()] = true;
			break;
		case ItemKind::Ring:
			worth = items.r_magic[obj->which<Ring>()].mi_worth;
			if (obj->which<Ring>() == Ring::AddStrength || obj->which<Ring>() == Ring::IncreaseDamage ||
				obj->which<Ring>() == Ring::Protection || obj->which<Ring>() == Ring::Dexterity)
			{
				if (obj->o_ac > 0)
					worth += obj->o_ac * 100;
				else
					worth = 10;
			}
			if (!obj->o_flags.test(ItemFlag::Known))
				worth /= 2;
			obj->o_flags.set(ItemFlag::Known);
			items.r_know[obj->which<Ring>()] = true;
			break;
		case ItemKind::Stick:
			worth = items.ws_magic[obj->which<Stick>()].mi_worth;
			worth += 20 * obj->charges();
			if (!obj->o_flags.test(ItemFlag::Known))
				worth /= 2;
			obj->o_flags.set(ItemFlag::Known);
			items.ws_know[obj->which<Stick>()] = true;
				break;
			case ItemKind::Amulet:
			worth = 1000;
			break;
	default:	// the other kinds of item: nothing
		break;
	}
	if (worth < 0)
		worth = 0;
	ui::display().write_at(c - 'a' + 1, 0,
		std::format("{}) {:5}  {}", static_cast<char>(c), worth, items::inv_name(*obj, false)));
	player.purse += worth;
	}
	ui::display().write_at(c - 'a' + 1, 0,
		std::format("   {:5}  Gold Pieces          ", static_cast<unsigned>(oldpurse)));
	score(player.purse, 2, 0);
	platform::md_exit(EXIT_SUCCESS);
}

/*
 * killname:
 *	Convert a code to a monster name
 */
std::string
killname(unsigned char monst, bool doart)
{
	std::string_view sp;
	bool article;

	article = true;
	switch (monst)
	{
	case 'a':
		sp = "arrow";
		break;
	case 'b':
		sp = "bolt";
		break;
	case 'd':
		sp = "dart";
		break;
	case 's':
		sp = "starvation";
		article = false;
		break;
	case 'f':
		sp = "fall";
		break;
	default:
		if (is_monster(monst))
			sp = entities::monsters[monst-'A'].m_name;
		else
		{
			sp = "God";
			article = false;
		}
	}
	if (doart && article)
		return std::format("a{} {}", vowelstr(sp), sp);
	return std::string(sp);
}

}  // namespace rogue
