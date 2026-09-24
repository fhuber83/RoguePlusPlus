#pragma once

#include "ui/Cell.hpp"
#include "ui/Input.hpp"

namespace rogue::ui {

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
