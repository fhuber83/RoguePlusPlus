#pragma once

#include <string>
#include <string_view>

/*
 * Game text is bytes (CP437 on screen, anything in rogue.opt), but JSON text
 * is UTF-8. The files write each byte as the code point of the same value
 * (Latin-1), so ASCII reads as itself and nothing is lost.
 */

namespace rogue::persistence {

// Each byte as the code point of the same value, in UTF-8
inline std::string bytes_to_utf8(std::string_view bytes)
{
	std::string out;
	for (unsigned char b : bytes) {
		if (b < 0x80)
			out += static_cast<char>(b);
		else {
			out += static_cast<char>(0xc0 | (b >> 6));
			out += static_cast<char>(0x80 | (b & 0x3f));
		}
	}
	return out;
}

// The reverse; a code point past U+00FF can't be a byte and reads as '?'
inline std::string utf8_to_bytes(std::string_view text)
{
	std::string out;
	for (std::size_t i = 0; i < text.size(); ) {
		auto b = static_cast<unsigned char>(text[i]);
		int length = b < 0x80 ? 1 : b < 0xe0 ? 2 : b < 0xf0 ? 3 : 4;
		if (length == 1)
			out += static_cast<char>(b);
		else if (length == 2 && b <= 0xc3 && i + 1 < text.size())
			out += static_cast<char>(((b & 0x1f) << 6) | (text[i + 1] & 0x3f));
		else
			out += '?';
		i += length;
	}
	return out;
}

}  // namespace rogue::persistence
