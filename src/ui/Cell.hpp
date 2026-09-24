#pragma once

#include <cstdint>

namespace rogue::ui {

/// DOS/CGA text attributes, as PC Rogue stored them next to each character in
/// video memory: bits 0-2 foreground colour, bit 3 bright, bits 4-6 background
/// colour, bit 7 blink.
namespace dos {

inline constexpr std::uint8_t Black = 0x00;
inline constexpr std::uint8_t Blue = 0x01;
inline constexpr std::uint8_t Green = 0x02;
inline constexpr std::uint8_t Red = 0x04;
inline constexpr std::uint8_t White = 0x07;
inline constexpr std::uint8_t Bright = 0x08;
inline constexpr std::uint8_t Blink = 0x80;

inline constexpr std::uint8_t Cyan = Blue | Green;
inline constexpr std::uint8_t Magenta = Blue | Red;
inline constexpr std::uint8_t Brown = Green | Red;
inline constexpr std::uint8_t Yellow = Brown | Bright;

/// Background colour bits for colour `c`.
constexpr std::uint8_t background(std::uint8_t c) { return static_cast<std::uint8_t>((c & 0x07) << 4); }

/// Light grey on black: the attribute after `standend()`.
inline constexpr std::uint8_t Normal = White;
/// Black on light grey. CGA had no reverse mode, so Rogue painted it.
inline constexpr std::uint8_t Standout = background(White);
/// Monochrome underline (MDA attribute 0x11).
inline constexpr std::uint8_t BwUnderline = Blue | background(Blue);
inline constexpr std::uint8_t BwStandout = Standout | Bright;

} // namespace dos

/// One character cell of the text screen.
struct Cell {
	std::uint8_t ch = ' ';           ///< CP437 code
	std::uint8_t attr = dos::Normal; ///< DOS attribute
	/// Drawn by the line/box routines. Terminals without the box-drawing
	/// glyphs pick a different ASCII fallback for these (`=` instead of `-`).
	bool line = false;

	friend bool operator==(const Cell &, const Cell &) = default;
};

} // namespace rogue::ui
