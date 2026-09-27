#pragma once

namespace rogue {

/*
 * Character tests and case conversions for ASCII, whatever the locale:
 * anything outside 0-127 (a negative char, a byte of 128 or more, a key
 * code) is none of these and converts to itself. They take an int, like
 * <cctype>, so a char, an unsigned char or a key all pass unchanged.
 */

constexpr bool is_upper(int c) { return c >= 'A' && c <= 'Z'; }
constexpr bool is_lower(int c) { return c >= 'a' && c <= 'z'; }
constexpr bool is_alpha(int c) { return is_upper(c) || is_lower(c); }
constexpr bool is_digit(int c) { return c >= '0' && c <= '9'; }
constexpr bool is_space(int c) { return c == ' ' || (c >= '\t' && c <= '\r'); }
constexpr bool is_print(int c) { return c >= ' ' && c <= '~'; }

constexpr int to_upper(int c) { return is_lower(c) ? c - 'a' + 'A' : c; }
constexpr int to_lower(int c) { return is_upper(c) ? c - 'A' + 'a' : c; }

}  // namespace rogue
