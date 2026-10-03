#include "ui/ScreenInput.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include "core/Ascii.hpp"
#include "ui/Cell.hpp"
#include "ui/Input.hpp"
#include "ui/Screen.hpp"

namespace rogue::ui {

namespace {

constexpr int Escape = 27;

} // namespace

int ScreenInput::read_key(int timeout_ms)
{
	return screen_.read_key(timeout_ms);
}

/*
 * This routine reads information from the keyboard
 * It should do all the strange processing that is
 * needed to retrieve sensible data from the user
 *
 * Changes from the original getinfo():
 * - Only printable ASCII chars are accepted
 */
std::optional<std::string> ScreenInput::read_line(std::size_t max_length)
{
	std::string line;
	bool wason = screen_.show_cursor(true);

	for (;;)
	{
		int ch;
		while ((ch = screen_.read_key(-1)) == key::None)
			;
		switch (ch)
		{
		case Escape:
			for (; !line.empty(); line.pop_back())
				backspace();
			screen_.show_cursor(wason);
			return std::nullopt;
		case key::Backspace:
		case '\b':
			if (!line.empty()) {
				backspace();
				line.pop_back();
			}
			break;
		case key::Enter:
		case '\n':
			screen_.show_cursor(wason);
			return line;
		default:
			if (line.size() >= max_length) {
				screen_.bell();
				break;
			}
			if (!is_print(ch))
				break;
			screen_.put(static_cast<std::uint8_t>(ch));
			line += static_cast<char>(ch);
			break;
		}
	}
}

/*
 * Step back and blank the character under the cursor, in the current
 * style as curses' winsch() did
 */
void ScreenInput::backspace()
{
	if (screen_.col() > 0)
		screen_.set_cursor(screen_.row(), screen_.col() - 1);
	screen_.set(screen_.row(), screen_.col(), Cell{' ', screen_.style(), false});
}

Input &input()
{
	static ScreenInput instance(screen());
	return instance;
}

} // namespace rogue::ui
