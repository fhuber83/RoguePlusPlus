/*
 * Reading and writing the score file. The original (rip.c) wrote its
 * struct sc_ent records to disk as they were in memory.
 */

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <optional>

#include <nlohmann/json.hpp>

#include "persistence/ByteText.hpp"
#include "persistence/HighScores.hpp"

namespace rogue::persistence {

namespace {

using json = nlohmann::ordered_json;	// keeps keys in the order written

constexpr std::string_view format_name = "rogue++ scores";
constexpr int format_version = 1;

// Titles in he_man[]: the score list shows he_man[experience - 1]
constexpr int max_experience = 21;

// struct sc_ent from rip.c, as it was written to disk
struct LegacyRecord {
	char sc_name[38];
	std::int32_t sc_rank;
	std::int32_t sc_gold;
	std::int32_t sc_fate;
	std::int32_t sc_level;
};
static_assert(sizeof(LegacyRecord) == 56);

std::string cut_name(std::string name)
{
	name.resize(std::min(std::strlen(name.c_str()), max_score_name));
	return name;
}

std::optional<int> get_int(const json &entry, const char *key)
{
	auto it = entry.find(key);
	if (it == entry.end() || !it->is_number_integer())
		return std::nullopt;
	auto value = it->get<long long>();
	if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
		return std::nullopt;
	return static_cast<int>(value);
}

std::optional<ScoreEntry> entry_from_json(const json &j)
{
	if (!j.is_object())
		return std::nullopt;
	auto name = j.find("name");
	auto gold = get_int(j, "gold");
	auto depth = get_int(j, "depth");
	auto experience = get_int(j, "experience");
	auto fate = get_int(j, "fate");
	if (name == j.end() || !name->is_string() || !gold || !depth || !experience || !fate)
		return std::nullopt;
	if (*experience < 1 || *experience > max_experience)
		return std::nullopt;

	ScoreEntry entry;
	entry.name = cut_name(utf8_to_bytes(name->get<std::string>()));
	entry.gold = *gold;
	entry.depth = *depth;
	entry.experience = *experience;
	entry.fate = *fate;
	return entry;
}

std::expected<std::vector<ScoreEntry>, ScoresError> parse_json(std::string_view bytes)
{
	json doc = json::parse(bytes, nullptr, false);
	if (doc.is_discarded() || !doc.is_object())
		return std::unexpected(ScoresError::BadFormat);
	auto format = doc.find("format");
	auto version = doc.find("version");
	auto scores = doc.find("scores");
	if (format == doc.end() || *format != format_name
	 || version == doc.end() || *version != format_version
	 || scores == doc.end() || !scores->is_array())
		return std::unexpected(ScoresError::BadFormat);

	std::vector<ScoreEntry> entries;
	for (const json &j : *scores) {
		auto entry = entry_from_json(j);
		if (!entry)
			return std::unexpected(ScoresError::BadFormat);
		entries.push_back(std::move(*entry));
	}
	return entries;
}

std::expected<std::vector<ScoreEntry>, ScoresError> parse_legacy(std::string_view bytes)
{
	if (bytes.size() % sizeof(LegacyRecord) != 0 || bytes.size() > max_scores * sizeof(LegacyRecord))
		return std::unexpected(ScoresError::BadFormat);

	std::vector<ScoreEntry> entries;
	for (std::size_t at = 0; at < bytes.size(); at += sizeof(LegacyRecord)) {
		LegacyRecord record;
		std::memcpy(&record, bytes.data() + at, sizeof record);
		if (record.sc_rank < 1 || record.sc_rank > max_experience)
			return std::unexpected(ScoresError::BadFormat);
		ScoreEntry entry;
		entry.name = cut_name(std::string(record.sc_name, sizeof record.sc_name));
		entry.gold = record.sc_gold;
		entry.depth = record.sc_level;
		entry.experience = record.sc_rank;
		entry.fate = record.sc_fate;
		entries.push_back(std::move(entry));
	}
	return entries;
}

}  // namespace

std::expected<ScoreList, ScoresError>
parse_scores(std::string_view bytes)
{
	auto start = bytes.find_first_not_of(" \t\r\n");
	ScoreList list;
	std::expected<std::vector<ScoreEntry>, ScoresError> entries;
	if (start == std::string_view::npos)
		entries = std::vector<ScoreEntry>{};
	else if (bytes[start] == '{')
		entries = parse_json(bytes);
	else {
		list.format = ScoresFormat::Legacy;
		entries = parse_legacy(bytes);
	}
	if (!entries)
		return std::unexpected(entries.error());

	std::erase_if(*entries, [](const ScoreEntry &e) { return e.gold <= 0; });
	std::ranges::stable_sort(*entries, std::ranges::greater{}, &ScoreEntry::gold);
	if (entries->size() > max_scores)
		entries->resize(max_scores);
	list.entries = std::move(*entries);
	return list;
}

std::string
format_scores(std::span<const ScoreEntry> entries)
{
	json scores = json::array();
	for (const ScoreEntry &e : entries)
		scores.push_back({
			{"name", bytes_to_utf8(e.name)},
			{"gold", e.gold},
			{"depth", e.depth},
			{"experience", e.experience},
			{"fate", e.fate},
			{"cause", bytes_to_utf8(e.cause)},
		});
	json doc = {
		{"format", format_name},
		{"version", format_version},
		{"scores", std::move(scores)},
	};
	return doc.dump(2) + "\n";
}

std::expected<ScoreList, ScoresError>
load_scores(const char *path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file)
		return std::unexpected(ScoresError::Unreadable);
	std::string bytes{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
	if (file.bad())
		return std::unexpected(ScoresError::Unreadable);
	return parse_scores(bytes);
}

bool
save_scores(const char *path, std::span<const ScoreEntry> entries)
{
	std::string temp = std::string(path) + ".tmp";
	{
		std::ofstream file(temp, std::ios::binary | std::ios::trunc);
		file << format_scores(entries);
		file.close();
		if (!file) {
			std::remove(temp.c_str());
			return false;
		}
	}
	if (std::rename(temp.c_str(), path) != 0) {
		std::remove(temp.c_str());
		return false;
	}
	return true;
}

}  // namespace rogue::persistence
