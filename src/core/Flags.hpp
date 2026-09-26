#pragma once

#include <type_traits>

namespace rogue {

/*
 * Opt-in marker: specialize to true for an enum whose enumerators are single
 * bits, to enable `A | B` on it and its use with Flags<E>.
 */
template <typename E>
inline constexpr bool enable_flags = false;

template <typename E>
concept FlagEnum = std::is_enum_v<E> && enable_flags<E>;

/*
 * A type-safe set of bit flags drawn from the enum E.
 *
 * Member names avoid the lowercase macros of the legacy headers (clear, move,
 * on, next, ...).
 *
 * The default constructor is trivial (like a plain integer field) so Flags can
 * live in aggregates and unions that are zero-initialized or memcpy'd.
 */
template <FlagEnum E>
class Flags {
public:
	using Bits = std::underlying_type_t<E>;

	Flags() = default;
	constexpr Flags(E flag) : bits_(static_cast<Bits>(flag)) {}

	static constexpr Flags none() { return from_bits(0); }
	static constexpr Flags from_bits(Bits bits)
	{
		Flags f;
		f.bits_ = bits;
		return f;
	}

	constexpr Bits bits() const { return bits_; }
	constexpr bool any() const { return bits_ != 0; }

	// True if any flag in `mask` is set.
	constexpr bool test(Flags mask) const { return (bits_ & mask.bits_) != 0; }

	constexpr Flags &set(Flags mask)
	{
		bits_ |= mask.bits_;
		return *this;
	}
	constexpr Flags &set(Flags mask, bool value) { return value ? set(mask) : unset(mask); }
	// not named clear(): the game's curses.h #defines clear
	constexpr Flags &unset(Flags mask)
	{
		bits_ &= static_cast<Bits>(~mask.bits_);
		return *this;
	}
	constexpr Flags &reset() { return *this = none(); }

	friend constexpr bool operator==(Flags a, Flags b) { return a.bits_ == b.bits_; }
	friend constexpr Flags operator|(Flags a, Flags b) { return from_bits(a.bits_ | b.bits_); }
	friend constexpr Flags operator&(Flags a, Flags b) { return from_bits(a.bits_ & b.bits_); }

private:
	Bits bits_;
};

}  // namespace rogue

/*
 * `A | B` for opted-in flag enums. Global so that it is found for enums in any
 * namespace; the FlagEnum constraint keeps it away from all other enums.
 */
template <rogue::FlagEnum E>
constexpr rogue::Flags<E>
operator|(E a, E b)
{
	return rogue::Flags<E>(a) | rogue::Flags<E>(b);
}
