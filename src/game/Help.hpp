#pragma once

/*
 * The help screens.
 *
 * Included by rogue.h after the legacy types (h_list).
 */

namespace rogue {

/*
 * help:
 *	Show a help table, a page at a time, until its end (an entry with no
 *	description) or Escape.
 */
void help(const h_list *helpscr);

}  // namespace rogue
