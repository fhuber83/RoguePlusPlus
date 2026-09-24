# Modernization Roadmap

Goal: turn the PC Rogue 1.48 C sources into modern, modular C++23. Gameplay, rules, monsters, items, rendering and input should each live in their own module.

## Decisions

| Topic | Decision |
|---|---|
| Strategy | **Incremental.** The game must build with zero warnings and stay playable after every step. |
| Fidelity | **Same rules, loosely.** Keep the game rules and feel. The RNG, the order of random calls and the save format may change. |
| DOS legacy | **Removed.** This covers copy protection, fake DOS, the BIOS/INT emulation, the title picture loader and the keyboard LED checks. |
| Language | **C++23** (GCC 15 / CMake 4). |

## Done

1. **DOS legacy stripped.**
   - The `ROGUE_DOS_*`, `ROGUE_NOGOOD`, `ROGUE_SPLASH`, `DEMO`, `INTL`, `LUXURY` and similar branches were resolved away.
   - Deleted `protect.c`, `fakedos.c`, `load.c`, `croot.c` and `swint.h`, plus the `sysint`/`swint`/`dmaout`/`peekb` stubs.
   - The copy-protection damage multiplier (`hit_mul`) and the "Software Pirate" tombstone are gone.
   - The status line now only shows the clock.
2. **Compiles as C++23.**
   - All sources are now `.cpp`.
   - K&R definitions were converted to prototypes, and `register` was removed.
   - String handling is `const`-correct: tables, struct fields, parameters and return types.
   - Fixed one undefined behaviour: the fungus attack wrote into a string literal. It now writes to the `f_damage` buffer.
3. **Foundations.**
   - CMake now builds a `rogue_game` static library, a thin `rogue++` executable (`src/app/main.cpp`) and a `rogue_tests` GoogleTest suite (v1.17.0 via FetchContent, run with `ctest`).
   - `src/` is a quote-only include path (`-iquote`) so that `src/curses.h` does not hide the system `<curses.h>`.
   - `core/Random`: `std::mt19937` with its own range mapping, so seeds reproduce on every platform. It replaces `seed`/`ran()`/`dnum`/`md_srand`. `rnd()`/`roll()` remain as inline wrappers around `rogue::rng()`.
   - `rogue++ -d <seed>` replays a dungeon, and the `v` command shows the seed.
   - Removed the stair-placement re-seed hack. It only existed because the old generator had short cycles.
   - `core/Dice` parses damage strings, and `roll_em` uses `parse_attacks()`. The venus flytrap's placeholder `"%%%d0"` became `"0d0"`.
   - `core/Coord` is now the `coord` type, with `==`, `+`, `-` and `distance_sq`. It replaces `ce()`/`_ce()`.
   - `core/Flags<E>` is an opt-in, type-safe bitset. Room flags are now `RoomFlags` (`RoomFlag::Dark/Gone/Maze`).
   - Fixed an original bug found by a test: the 13th entry of `passages[]` was never initialized as a dark corridor.
   - **Deferred to phase 6:** item kinds as an `enum class`, and creature/object flags as `Flags`. Item kinds double as map glyphs (`POTION == '!'`), and both flag sets live in `union thing`, so they move together with the THING split.
4. **UI seam.** Game logic draws only through `ui::Display` and reads keys only through `ui::Input`.
   - **4.1 Screen and terminal.**
     - `ui::Screen` is an 80×25 grid of CP437 cells with a cursor and a current DOS attribute, standing in for PC Rogue's video memory. Writes go to the grid and through to a connected `ui::Terminal`. Reads come from the grid.
     - `mvinch`/`inch` read-back is now exact. The curses port reverse-mapped terminal characters, and in ASCII mode it could not tell room corners from walls.
     - `curses.cpp` became `ui/curses/CursesTerminal.cpp`, which only paints cells (charset and colour mapping) and reads keys, returned as `ui::key` values.
     - The DOS screen API the game calls (`cur_*`, `set_attr`, `wdump`/`wrestor`, boxes, curtains, `getinfo`, key translation) moved to `ui/DosScreen.cpp` on top of `Screen`. The local `curses.h` macros are unchanged, so game files did not change.
     - `LINES`/`COLS` are now constants in the local `curses.h` instead of ncurses' globals. `ROGUE_COLUMNS` and the unused `keypad.h` are gone.
     - Verified by replaying four seeds with the same keystrokes on the old and new builds. All 360 tmux captures, colours included, were identical. `tests/ui/ScreenTest.cpp` covers the grid headlessly.
   - **4.2 Message and status lines.**
     - `ui::Display` (`ui/Display.hpp`) is the game-facing output interface. So far it covers the message line (`draw_message`, `clear_message`, and `show_more`/`blink_more`/`hide_more` for the More and Cont prompts), the status lines (`draw_status(ui::Status)`) and the clock (`draw_clock`).
     - `ui::ScreenDisplay` implements it on a `Screen` with the original layout. `ui::display()` returns the game's instance.
     - `io.cpp` keeps the logic: formatting, capitalisation, the `huh` history, `look()` before More, waiting for the space key, splitting long messages, and building `ui::Status` from game state. It no longer draws.
     - Small fixes:
       - Hits and Str now also redraw when only the maximum changes.
       - The More prompt restores the exact cells it covered. The old code rewrote them as plain text and repeated column 78 in column 79.
     - `scrlmsg()` is gone. Its sideways scroll was never refreshed, so only the last frame was ever visible. The 40-column status positions (`PT()`) are gone too.
     - `wait_msg`, `show_win` and `str_attr` still draw directly. They move with the full-screen views in 4.4.
     - The same A/B replay gave identical captures. `tests/ui/ScreenDisplayTest.cpp` covers the layout.
   - **4.3 Map.**
     - Game code draws the map with `display().draw_tile(Coord, glyph, TileStyle)` and reads the hero's view back with `display().tile_at(Coord)`.
     - `TileStyle` (`Normal`, `Inverse`, `Bolt`, `FrostBolt`) replaces the `standout()`/`blue()`/`red()` brackets. Inverse covers passages and mazes, sensed monsters, detected items and the teleport flash.
     - The repeat count, animation refreshes and the trap bell go through `draw_count`, `flush` and `bell`. `rogue.h` includes `ui/Display.hpp` and brings `display` and `TileStyle` into scope.
     - Domain files no longer call `mvaddch`, `mvinch`, `standout` or the colour macros. The exceptions are the full-screen views in `misc.cpp` (help), `things.cpp` (inventory) and `wizard.cpp` (`show_map`), plus `implode()` in `new_leve.cpp`, which all belong to 4.4.
     - Verification: the A/B replay, a fuzz replay over six seeds, and scripted item scenarios on `WIZARD` builds (`C` creates items). All captures were identical. Frame-by-frame captures of zapped wands showed the same red bolt in both builds. `WIZARD` builds do not compile as is, so both trees were patched in scratch copies for this (see the notes below).
   - **4.4a In-game pages and prompts.**
     - `Display` gained a page API. `open_page`/`close_page` keep and restore the game view, `page_open()` pauses the clock and `clear_page` blanks the screen. Text goes through `write_at`/`write` with a named `ui::Ink` style (the old colour macros by name) and `clear_line`, plus `show_cursor` and `wipe` (the `implode` effect).
     - Converted: help (`misc.cpp`), inventory and discoveries (`add_line`/`end_line` in `things.cpp`), the wizard map, `show_win`, `str_attr`, `wait_msg`, the quit prompt and `leave()` (`playit.cpp`), and the new-level wipe.
     - Verified with the A/B, fuzz and quit/page replays: identical apart from one capture taken in the middle of a curtain animation.
   - **4.4b Title and ending screens.**
     - `Display` draws whole screens: `draw_title`/`end_title` (credits and the name prompt), `draw_tombstone`, `draw_scores` (the game fills in `ui::ScoreLine` values with the rank and fate text) and `draw_winner`, plus `curtain_down`/`curtain_up` and `wipe`. `ScreenDisplay` holds the artwork that used to live in `mach_dep.cpp`, `rip.cpp` and `curses.cpp`.
     - `ui::start_terminal()`/`ui::stop_terminal()` replace `winit()`/`cur_endwin()` in `main`, `fatal` and `md_exit`.
     - `score()` opens a page instead of setting `is_saved`. The clock now pauses only for open pages. `is_saved` and the DOS `implode`, curtain, box, `center` and `repchr` code are gone.
     - `save.cpp` is down to its two stubs. The dead memory-dump save and restore code was deleted, since phase 8 replaces it.
     - Verified with the A/B and quit replays, a crafted `rogue.scr` covering every fate in the Hall of Fame (`-s`), and captures of the credits and name entry. All identical apart from one mid-curtain frame.
   - **4.5 Input.**
     - `ui::Input` (`ui/Input.hpp`) has `read_key(timeout)` (characters or `ui::key` values) and `read_line`, which replaces `getinfo`. `ui::ScreenInput` implements it on the `Screen` connected to the terminal, and `ui::input()` returns the game's instance.
     - The key-to-command table (F1 to `?`, arrows to `hjkl`, ...) lives next to `readchar()` in `mach_dep.cpp` again.
     - Game files no longer include the local `curses.h`, which is deleted. The glyph codes, screen size and key constants they need are in `glyphs.h`, which `rogue.h` includes. `LINES`/`COLS` are constants in `rogue.h`.
     - `ui/DosScreen.cpp` keeps only the DOS attribute tables, glyph colouring and terminal start and stop. `curses_common.h` is private to `ui/`.
     - Verified with the A/B, quit and fuzz replays and a name-editing replay (typing, backspace, Enter): identical. `tests/ui/ScreenInputTest.cpp` covers line editing.
   - **4.6 DOS emulation removed.**
     - Cells hold a glyph code and a `ui::Style` (foreground and background `ui::Color`, blink, underline) instead of a DOS attribute byte.
     - `ScreenDisplay` owns the colour policy: the style of each `Ink`/`TileStyle`, the colours map glyphs get, and a monochrome switch (`SCREEN=bw` in `rogue.opt`, or a terminal without colours).
     - `CursesTerminal` renders a cell from one glyph table (code → ASCII, box-line ASCII, Unicode) and turns a style into curses attributes and a colour pair. It requires wide-character curses (CMake already did).
     - Gone: `ui/DosScreen.cpp`, `curses_common.h`, `ui/curses/curses_dos.h`, the three-way CCODE tables, the narrow-character path, `scr_type`/`is_color` (it was always 80×25 colour) and the CP437 pass-through charset. `ROGUE_CHARSET=1` still selects ASCII; any other value means Unicode.
     - Monochrome underline is a real underline now. The old table asked for blue on blue.
     - Glyph codes stay CP437 bytes, because game logic compares them and they double as item kinds. Separating them is phase 6.
     - Verified with the A/B, quit, fuzz, name-editing, Hall of Fame and monochrome (`SCREEN=bw`) replays: identical apart from mid-curtain frames.

5. **Game state** (in progress). Globals move into `rogue::Game` (`game/Game.hpp`), reached through `rogue::game()` the way `display()` is. `rogue.h` includes it after the legacy types it holds, so game files keep including only `rogue.h`. Each group of globals is deleted and the compiler finds every use, which leaves locals that shadow a global alone.
   - **5.1 Options.**
     - `game().options` (`rogue::Options`) holds what `rogue.opt`, the name prompt and the in-game toggles set: `name` (was `whoami`), `fruit`, `macro`, `score_file`, `save_file`, `drive`, `menu`, `screen`, `monochrome` (was `bwflag`), `terse` and `expert`. The buffers keep their original sizes. `brief()` replaces the repeated `terse || expert`.
     - `env.cpp` builds its label table per call, since it now points into the game.
     - Deleted dead globals: `revno`/`verno` (the `v` command prints `REV`/`VER`), `maxitems` (written, never read), `reinit` (never set) and `_whoami`.
     - Verified with the A/B replay over four seeds plus two seeds with a `rogue.opt` that sets every option (name, fruit, macro, `menu=sel`, `screen=bw`, score file). The replays now end on the score screen with a pre-created score file, and the score files are compared too. All identical apart from one mid-curtain frame. `tests/game/GameTest.cpp` covers the defaults and reading `rogue.opt`.
   - **5.2 Messages and command state.**
     - `game().message` (`rogue::MessageLine`) holds the message being built (`text`, was the allocated `msgbuf`), the last one for ^R (`last`, was `huh`), where the shown and the next message end (`end`/`next_end`, were `mpos` and `io.cpp`'s `newpos`) and `remember` (was `save_msg`).
     - `game().turn` (`rogue::Turn`) holds the state of the command being carried out: `after`, `again`, `count`, `take`, `running`, `run_dir` (was `runch`), `door_stop`, `first_move`, `fast_mode`, `fast_state`, `delta`, `typeahead` (was `typebuf`), `bailout`, and the repeat memory that were `command.cpp` statics (`last_count`, `last_ch`, `last_take`, `do_take`).
     - `game().playing` and `game().noscore`.
     - Functions that use a group often bind a local reference (`rogue::Turn &turn = game().turn;`).
     - The A/B replay now also covers repeat counts, `a`, `g` and `f` prefixes, and defining and running the F9 macro. Identical.
   - **5.3 The player.**
     - `game().player` (`rogue::Player`) holds the rogue: `body` (the THING that was `player`, so `hero`, `pstats`, `pack`, `proom` and `max_hp` now expand to `game().player.body...`), `max_stats`, `purse`, `in_pack`, `armor`/`weapon`/`rings[2]` (were `cur_armor`, `cur_weapon`, `cur_ring`), food and hunger, `has_amulet` (was `amulet`), `saw_amulet`, `max_level`, `no_command`, `no_move`, `quiet`, `fung_hit`, `was_trapped`, and `look()`'s `old_pos`/`old_room` (were `oldpos`/`oldrp`).
     - `e_levels` stays a fixed table in `init.cpp`, since it is the same in every game.
     - Verified with the A/B replay: identical.

## Target architecture

```
src/
  core/         Coord, Random (seedable, injectable), Dice ("2d4" -> struct), bitflag enums
  world/        Level (tile grid + flags), Room, Passage, LevelGenerator (rooms/passages/maze/new_leve)
  entities/     Creature, Player, Monster, MonsterCatalog (monsters[]), MonsterAI (chase, slime, wander)
  items/        Item, ItemKind, Inventory (pack), ItemCatalog + Identification (names/guesses/know),
                effects: Potion, Scroll, Wand, Ring, Armor, Weapon
  rules/        Combat (fight), Scheduler (daemons + fuses), Hunger/Regeneration
  game/         Game (owns all state that is global today), Command enum, CommandDispatcher
  ui/           Display + Input interfaces; curses/ implementation (map view, status line,
                message log, screens: inventory, help, discoveries, tombstone, scores)
  persistence/  Options (rogue.opt), HighScores, SaveGame (real serialization)
```

Dependency rule: `ui` → `game` → (`rules`, `entities`, `items`, `world`) → `core`. Game logic never includes curses. It reports what happens (messages, "tile changed", "show inventory") through the `Display` interface and gets `Command`s from `Input`. A headless `Display`/`Input` pair then allows scripted play tests.

## Phases

Each phase is a series of small commits that each build and play.

4. **UI seam** (*done*, see above). Steps:
   1. *Done:* `ui::Screen` grid plus `ui::Terminal` backend (see above).
   2. *Done:* message and status lines behind `ui::Display` (see above).
   3. *Done:* the map goes through `Display` (see above).
   4. *Done:* full-screen views, in-game pages and prompts (4.4a), title and ending screens (4.4b).
   5. *Done:* input behind `ui::Input`, and no game file includes the DOS screen API.
   6. *Done:* the DOS emulation is gone (see above).
5. **Game state** (*in progress*, see above). Gather the ~90 globals from `extern.cpp`/`init.cpp` into a `Game` context (player, level, monster list, floor items, RNG, scheduler, known-item tables, options). Free functions take or reach it explicitly, and globals are removed one group at a time. Steps:
   1. *Done:* options (see above).
   2. *Done:* messages and command state (see above).
   3. *Done:* the player (see above).
   4. The level: `level`, `rooms`, `passages`, the map and flag grids, floor items and monsters, traps.
   5. Items: the known-item and guess tables, per-game names, colours, stones and materials, the probability tables that `init_*()` accumulates, and the item pool.
   6. The scheduler (`daemon.cpp` slots) and the RNG.
   Algorithm scratch state (`maze.cpp`, `passages.cpp`, `ch_ret`, ...) and fixed tables stay where they are until phase 7.
6. **Entities.**
   - Split `union thing` into `Monster` and `Item`.
   - Item kinds become an `enum class` with a separate glyph mapping, and creature/object flags become `rogue::Flags`.
   - Replace the intrusive `l_next`/`l_prev` lists (`list.cpp`) with standard containers of `std::unique_ptr` and stable IDs.
   - Replace the `#define t_pos _t._t_pos` accessor macros with members.
7. **Domain modules.**
   - Move behaviour into the target directories: item effects become per-kind handlers, `fight` becomes `Combat`, `chase` becomes `MonsterAI`, and the level generation files become `LevelGenerator`.
   - Scheduler: replace `daemon.cpp` with typed events or `std::function` rather than function-pointer slots.
   - Commands: `command.cpp` becomes `CommandDispatcher` over a `Command` enum.
8. **Persistence.**
   - Options loader.
   - High scores as a real file format.
   - Save/restore: the original was a raw memory dump and is currently disabled. Replace it with serialization of `Game`.
9. **Idiom cleanup.**
   - `std::string`/`std::format` instead of `sprintf` into `prbuf`.
   - Remove `when`/`otherwise`/`on()`/`ce()`-style macros and `shint`/`byte` typedefs.
   - Remove the `//@` port annotations once the code they describe is gone.

## Notes for whoever continues

- `score()` writes `sc_name[38]` to `rogue.scr` with uninitialized bytes after the name. Harmless, but compare score files by the name up to its NUL. Phase 8 replaces the format.

- `faststate` ("Fast Play") used to be toggled by Scroll Lock and is now always `FALSE`. Reintroduce it as a real option or key if wanted.
- `save_game()` prints "saving games is disabled" and `restore()` exits with a message. Phase 8 brings real saving.
- `WIZARD` builds do not compile: `CTRL(D)` in `command.cpp` should be `CTRL('D')`, `rogue.h` defines `bool wizard;` in the header (it should be `extern`, while `extern.cpp` defines it only under `WIZARD`), and `create_obj()` passes a `short *` and `stdscr` to `get_num(int *)`.
- The terminal must be 80×25. `COLS == 40` paths still exist for the old 40-column mode.
