#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <utility>

namespace rogue {

/*
 * How many kinds the enum E numbers from 0. Specialized next to each enum
 * that indexes a KindTable. An enumerator at or past the count is not a real
 * kind (WeaponType::Flame, Stick::Vorpal).
 */
template <typename E>
inline constexpr std::size_t kind_count = 0;

template <typename E>
concept KindEnum = std::is_enum_v<E> && (kind_count<E> > 0);

/*
 * A table with one entry per kind of E, indexed by the enum rather than a
 * number, so a potion can't look itself up in the scroll table. N is larger
 * than kind_count<E> only for a table that has entries for the fake kinds.
 *
 * Like the C array it replaces, it is trivially copyable when T is, and
 * value-initialized (`= {}`) it is zeroed. A table given entries must have exactly N of them,
 * checked at compile time.
 */
template <KindEnum E, typename T, std::size_t N = kind_count<E>>
class KindTable {
public:
	constexpr KindTable() = default;
	consteval KindTable(std::initializer_list<T> entries)
	{
		if (entries.size() != N)
			throw "a KindTable needs one entry per kind";
		std::copy(entries.begin(), entries.end(), values_.begin());
	}

	constexpr T &operator[](E kind) { return values_[std::to_underlying(kind)]; }
	constexpr const T &operator[](E kind) const { return values_[std::to_underlying(kind)]; }

	static constexpr std::size_t size() { return N; }
	constexpr T *data() { return values_.data(); }
	constexpr const T *data() const { return values_.data(); }
	constexpr auto begin() { return values_.begin(); }
	constexpr auto begin() const { return values_.begin(); }
	constexpr auto end() { return values_.end(); }
	constexpr auto end() const { return values_.end(); }

private:
	std::array<T, N> values_;
};

// Each kind of E in order, for a range-for: for (Ring r : kinds<Ring>())
template <KindEnum E>
constexpr auto
kinds()
{
	std::array<E, kind_count<E>> all;
	for (std::size_t i = 0; i < all.size(); i++)
		all[i] = static_cast<E>(i);
	return all;
}

}  // namespace rogue
