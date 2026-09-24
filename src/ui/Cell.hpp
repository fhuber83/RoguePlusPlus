#pragma once

#include <cstdint>

namespace rogue::ui {

/// The 16 text colours of the IBM PC (CGA), in their hardware order:
/// bit 0 blue, bit 1 green, bit 2 red, bit 3 bright.
enum class Color : std::uint8_t {
	Black,
	Blue,
	Green,
	Cyan,
	Red,
	Magenta,
	Brown,
	LightGrey,
	DarkGrey,
	LightBlue,
	LightGreen,
	LightCyan,
	LightRed,
	LightMagenta,
	Yellow,
	White,
};

/// How a cell is drawn. Backgrounds use the first 8 colours only.
struct Style {
	Color fg = Color::LightGrey;
	Color bg = Color::Black;
	bool blink = false;
	bool underline = false;

	friend bool operator==(const Style &, const Style &) = default;
};

/// Light grey on black.
inline constexpr Style Plain{};
/// Black on light grey. CGA had no reverse mode, so Rogue painted it.
inline constexpr Style Standout{Color::Black, Color::LightGrey};

/// One character cell of the text screen.
struct Cell {
	std::uint8_t ch = ' '; ///< glyph code (CP437, see glyphs.h)
	Style style;
	/// Drawn by the line/box routines. Terminals without the box-drawing
	/// glyphs pick a different ASCII fallback for these (`=` instead of `-`).
	bool line = false;

	friend bool operator==(const Cell &, const Cell &) = default;
};

} // namespace rogue::ui
