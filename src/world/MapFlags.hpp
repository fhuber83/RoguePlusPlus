#pragma once

#include "core/Flags.hpp"
#include "world/Trap.hpp"

namespace rogue {

/*
 * What the map knows about a square beyond what is drawn there (the F_*
 * defines). The low bits are not flags: a passage square holds its passage's
 * number, a trap square the kind of trap.
 */
enum class MapFlag : unsigned char {
	Real    = 0x10,	/* F_REAL: what you see is what you get (not a secret door or hidden trap) */
	Maze    = 0x20,	/* F_MAZE: a square of a maze */
	Passage = 0x40,	/* F_PASS: a square of a passage */
};
template <>
inline constexpr bool enable_flags<MapFlag> = true;

/*
 * A square's map flags, and its passage number or trap kind, in one byte (the
 * byte the save file stores). Like Flags, it is trivially default
 * constructible, so the level's grid of them can be zeroed and copied.
 */
class MapFlags {
public:
	MapFlags() = default;
	constexpr MapFlags(MapFlag flag) : bits_(static_cast<unsigned char>(flag)) {}
	constexpr MapFlags(Flags<MapFlag> flags) : bits_(flags.bits()) {}

	static constexpr MapFlags from_bits(unsigned char bits)
	{
		MapFlags f;
		f.bits_ = bits;
		return f;
	}
	constexpr unsigned char bits() const { return bits_; }

	// True if any flag in `mask` is set.
	constexpr bool test(Flags<MapFlag> mask) const { return (bits_ & mask.bits()) != 0; }
	constexpr MapFlags &set(Flags<MapFlag> mask)
	{
		bits_ |= mask.bits();
		return *this;
	}
	constexpr MapFlags &unset(Flags<MapFlag> mask)
	{
		bits_ &= static_cast<unsigned char>(~mask.bits());
		return *this;
	}

	/*
	 * The number of the passage this square belongs to, an index into
	 * Level::passages (F_PNUM). 0 until numpass() has numbered it; the first
	 * passage it numbers is 1.
	 */
	constexpr int passage() const { return bits_ & passage_mask; }
	constexpr void set_passage(int number)
	{
		bits_ = static_cast<unsigned char>((bits_ & ~passage_mask) | number);
	}

	// The kind of trap on this square, if it is a trap (F_TMASK)
	constexpr Trap trap() const { return static_cast<Trap>(bits_ & trap_mask); }
	constexpr void set_trap(Trap trap)
	{
		bits_ = static_cast<unsigned char>((bits_ & ~trap_mask) | static_cast<unsigned char>(trap));
	}

private:
	static constexpr unsigned char passage_mask = 0x0f;
	static constexpr unsigned char trap_mask = 0x07;

	unsigned char bits_;
};

}  // namespace rogue
