#pragma once

namespace rogue::ui {

/// Values read_key() returns besides plain characters (0-255).
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

/// Where the game gets keys from.
class Input {
public:
	virtual ~Input() = default;

	/// Next key: a character (0-255) or a ui::key value, or key::None after
	/// `timeout_ms` milliseconds. A negative timeout waits for a key.
	/// Whatever was drawn becomes visible first.
	virtual int read_key(int timeout_ms) = 0;

	/// Lets the player type up to `size` printable characters at the
	/// cursor, echoing them, with backspace. `buf` needs room for `size`
	/// characters and a terminating '\0'.
	/// Returns '\n' (or key::Enter) when done. Escape clears what was typed,
	/// leaves ESCAPE and '\0' in `buf` and returns ESCAPE.
	virtual int read_line(char *buf, int size) = 0;
};

/// The input the game uses.
Input &input();

} // namespace rogue::ui
