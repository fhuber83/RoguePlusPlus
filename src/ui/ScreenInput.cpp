#include "ui/ScreenInput.hpp"

#include <cctype>

#include "ui/Terminal.hpp"

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
 * - Aborted input is null-terminated (ESCAPE + '\0')
 * - Only printable ASCII chars are accepted
 */
int ScreenInput::read_line(char *buf, int size)
{
	char *str = buf;
	int ch;
	int readcnt = 0;
	int ret = 1;
	bool wason;

	*str = 0;
	wason = screen_.show_cursor(true);
	while (ret == 1)
	{
		while ((ch = screen_.read_key(-1)) == key::None)
			;
		switch (ch)
		{
		case Escape:
			while (str != buf) {
				backspace();
				readcnt--;
				str--;
			}
			ret = *str++ = Escape;
			*str = 0;
			screen_.show_cursor(wason);
			break;
		case key::Backspace:
		case '\b':
			if (str != buf) {
				backspace();
				readcnt--;
				str--;
			}
			break;
		default:
			if (readcnt >= size) {
				screen_.bell();
				break;
			}
			if (ch > 0x7f || !std::isprint(ch))
				break;
			readcnt++;
			screen_.put(static_cast<std::uint8_t>(ch));
			*str++ = static_cast<char>(ch);
			break;
		case key::Enter:
		case '\n':
			*str = 0;
			screen_.show_cursor(wason);
			ret = ch;  // any value different than ESCAPE or 1 would do.
			break;
		}
	}
	return ret;
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
