#pragma once

#include <cstddef>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <vector>

/*
 * The score file (rogue.scr, or scorefile= in rogue.opt): the ten best
 * games, richest first, as JSON:
 *
 *	{
 *	  "format": "rogue++ scores",
 *	  "version": 1,
 *	  "scores": [
 *	    { "name": "Rodney", "gold": 312, "depth": 6, "experience": 5,
 *	      "fate": 75, "cause": "killed by a kestrel" },
 *	    ...
 *	  ]
 *	}
 *
 * "fate" is what the game reads back: a monster's letter ('K' for the
 * kestrel above; also 'a' arrow, 'b' bolt, 'd' dart, 'f' fall, 's'
 * starvation), 1 for quit, 2 for a total winner. "cause" says the same in
 * words; it is only written.
 *
 * Names are byte strings (rogue.opt may hold any byte, and the screen draws
 * them as CP437). Each byte is written as the code point of the same value,
 * so ASCII reads as itself and nothing is lost.
 *
 * Reading also takes the original binary format (up to ten 56-byte records
 * as the C struct sc_ent laid them out), so an old score file is kept and
 * written back as JSON.
 */

namespace rogue::persistence {

inline constexpr std::size_t max_scores = 10;
inline constexpr std::size_t max_score_name = 37;

struct ScoreEntry {
	std::string name;
	int gold = 0;
	int depth = 0;			// Deepest level reached (sc_level)
	int experience = 0;		// Experience level, picks the title (sc_rank)
	int fate = 0;			// sc_fate, see above
	std::string cause;		// Written for people, ignored when read
};

enum class ScoresFormat {
	Json,
	Legacy,					// The original binary records
};

struct ScoreList {
	std::vector<ScoreEntry> entries;	// Richest first, at most max_scores
	ScoresFormat format = ScoresFormat::Json;
};

enum class ScoresError {
	Unreadable,				// The file could not be opened or read
	BadFormat,				// Neither valid score JSON nor the old records
};

/*
 * parse_scores:
 *	Read a score file's contents. Empty (or blank) contents are an empty
 *	list. Entries without gold are dropped, and the rest are sorted richest
 *	first (keeping the order of equals) and cut to max_scores.
 */
std::expected<ScoreList, ScoresError> parse_scores(std::string_view bytes);

/*
 * format_scores:
 *	The JSON text of a score file holding entries.
 */
std::string format_scores(std::span<const ScoreEntry> entries);

/*
 * load_scores:
 *	Read and parse the score file at path.
 */
std::expected<ScoreList, ScoresError> load_scores(const char *path);

/*
 * save_scores:
 *	Write entries to the score file at path, through a temporary file
 *	renamed over it, so a failed write leaves the old file. Returns false
 *	if it could not be written.
 */
bool save_scores(const char *path, std::span<const ScoreEntry> entries);

}  // namespace rogue::persistence
