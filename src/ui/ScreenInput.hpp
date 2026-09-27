#pragma once

#include "ui/Input.hpp"
#include "ui/Screen.hpp"

namespace rogue::ui {

/// Input from the terminal connected to a Screen. Typed lines are echoed
/// on that screen.
class ScreenInput final : public Input {
public:
	explicit ScreenInput(Screen &screen) : screen_(screen) {}

	int read_key(int timeout_ms) override;
	std::optional<std::string> read_line(std::size_t max_length) override;

private:
	void backspace();

	Screen &screen_;
};

} // namespace rogue::ui
