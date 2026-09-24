#pragma once

#include "ui/Cell.hpp"

namespace rogue::ui {

/// Values returned by Terminal::read_key() besides plain characters (0-255).
namespace key {

inline constexpr int None = -1; ///< timeout, resize, or a key the game has no use for

inline constexpr int Enter = 0x100;
inline constexpr int Home = 0x101;
inline constexpr int Up = 0x102;
inline constexpr int PageUp = 0x103;
inline constexpr int Backspace = 0x104;
inline constexpr int Left = 0x105;
inline constexpr int Right = 0x106;
inline constexpr int End = 0x107;
inline constexpr int Down = 0x108;
inline constexpr int PageDown = 0x109;
inline constexpr int Insert = 0x10a;
inline constexpr int Delete = 0x10b;
inline constexpr int F1 = 0x110; ///< F1 to F9 are consecutive
inline constexpr int F9 = F1 + 8;
inline constexpr int AltF9 = 0x120;

/// Function key `n` (1-9).
constexpr int function(int n) { return F1 + n - 1; }

} // namespace key

/// Backend that shows a Screen and reads the keyboard.
///
/// Output is buffered: draw() and set_cursor() take effect on the next
/// flush(). read_key() flushes first, so whatever was drawn is visible
/// while the game waits for a key.
class Terminal {
public:
	virtual ~Terminal() = default;

	virtual void draw(int row, int col, const Cell &cell) = 0;
	virtual void set_cursor(int row, int col) = 0;
	virtual void show_cursor(bool visible) = 0;
	virtual void flush() = 0;
	virtual void bell() = 0;

	/// Next key, or key::None after `timeout_ms` milliseconds. A negative
	/// timeout waits for a key.
	virtual int read_key(int timeout_ms) = 0;
};

} // namespace rogue::ui
