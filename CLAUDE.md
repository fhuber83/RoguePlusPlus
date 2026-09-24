# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

RoguePlusPlus is a C++ port of the classic PC game "Rogue", based on the PC Rogue 1.48 sources (A.I. Design / Epyx, 1983–84) as ported to Linux/ncurses. The code now compiles as **C++23**, but it is still structured like the original C: free functions, global state, and `union thing`. It is being modernized incrementally into separate modules. The plan, the decisions already made and the progress so far are in **`docs/MODERNIZATION.md`**. Read it before any structural change, and update its "Done" section when a phase step lands.

## Build & run

Requirements: CMake ≥ 4.0, a C++23 compiler (GCC 15 is used), pkg-config, and ncursesw.

```sh
cmake -S . -B build             # first configure fetches GoogleTest (ROGUE_BUILD_TESTS=OFF to skip)
cmake --build build
ctest --test-dir build          # all unit tests
./build/rogue_tests --gtest_filter='Dice.*'   # a single suite or test
./build/rogue++                 # new game
./build/rogue++ -d 4242         # reproducible dungeon from a seed (`v` in game shows the seed)
./build/rogue++ -s              # show scores
```

Targets: `rogue_game` is a static library built from every `src/**/*.cpp` except `src/app/`. `rogue++` links `src/app/main.cpp` against it. `rogue_tests` builds from `tests/**/*.cpp`, and tests may include any game header. Sources are globbed with `CONFIGURE_DEPENDS`. All targets use `-Wall -Wextra -Wpedantic` and **must stay warning-free**.

Smoke test without a real terminal: `tmux new-session -d -s rg -x 80 -y 25 ./build/rogue++`, then `tmux send-keys -t rg ...` and `tmux capture-pane -p -t rg`. The game needs an 80×25 screen. After the name prompt, wait about 2.5 s for the curtain animation to finish.

At runtime the game reads `rogue.opt` (options) and writes `rogue.scr` (scores) in the current working directory. Saving is disabled: `save_game()` and `restore()` are stubs.

## Conventions

- New code goes in `namespace rogue`, in `.hpp`/`.cpp` files under a module directory (`src/core/`, …), and is included as `"core/Random.hpp"`. `src/` is on the include path only for quoted includes (`-iquote`), so project headers never hide system headers.
- Legacy headers define many lowercase macros (`on`, `next`, `prev`, `pack`, `hero`, `max`, `max_hp`, `attach`, `detach`, `when`, …). Include standard and modern headers **before** `rogue.h`/`extern.h`, as `rogue.h` does for `core/` and `ui/`. Never name members after those macros. That's why `Flags` has `unset()` rather than `clear()`.
- All randomness goes through `rogue::rng()` (or the `rnd()`/`roll()` wrappers). Never use `rand()` or the clock, or seeds stop reproducing.
- **`//@` and `/*@` comments** in legacy files mark changes made by the Linux port and this project. Everything else there is original 1980s code.
- Keep string handling `const`-correct. Buffers the game really writes to (`prbuf`, `f_damage`, `s_names`, `_guesses`, …) are `char[]`. Everything else is `const char *`.

## Architecture (current, pre-modularization)

- **Core types** (`src/core/`): `Random`, `Coord` (the legacy `coord` is an alias), `Dice` (parses damage strings like `"1d2/1d5"`), and `Flags<E>`, a bitset for opted-in enums (`RoomFlag`/`RoomFlags` so far).
- **Game state** (`src/game/`): `rogue::Game`, reached through `game()`, gathers the former globals group by group (phase 5; only `options` so far). The rest still live in `extern.cpp`/`init.cpp`.
- **Headers**:
  - `extern.h`: libc includes, POSIX feature macros, and libc "overrides": `#define access(f) access(f, F_OK)`, `stpchr`, `setmem`/`bcopy`. Remember these when a libc call behaves unexpectedly.
  - `rogue.h`: game constants, structs, globals, prototypes, plus accessor macros like `#define t_pos _t._t_pos` over `union thing` (THING).
  - `extern.cpp`/`init.cpp`: define most of the globals.
- **UI layer** (`src/ui/`): game code talks only to two interfaces, both reachable through `rogue.h`:
  - `display()` (`ui/Display.hpp`): the message line, status and clock, map tiles (`draw_tile`/`tile_at` with a `TileStyle`), pages (`open_page`/`write_at` with an `Ink`), and the title, tombstone, score and winner screens.
  - `input()` (`ui/Input.hpp`): `read_key` (characters or `ui::key` values) and `read_line`.
  - Implementations: `ScreenDisplay` and `ScreenInput` work on `ui::Screen`, an 80×25 grid of cells (glyph code + `ui::Style`). Reads such as `tile_at` come from this grid. `ScreenDisplay` owns layout and colour policy (styles per `Ink`/`TileStyle`, glyph colours, monochrome). `ui::Terminal` is the backend interface, implemented by `ui/curses/CursesTerminal.cpp`, the only file that includes the system `<curses.h>`. That file also holds `ui::start_terminal()`/`stop_terminal()`.
  - `glyphs.h` has the glyph codes (CP437 bytes, which double as item kinds until phase 6), key constants and string/screen sizes shared by game and UI. **Game files must not draw or read the terminal any other way, and must never include `<curses.h>`.**
- **Machine layer**: `mach_dep.cpp` holds time, sleep, `readchar`, `newmem`, `fatal`/`md_exit`, and the credits screen.
- **Game loop**: `app/main.cpp` parses arguments, seeds `rng()`, and sets up the game with `init_*()` → `new_level()`, then starts daemons and fuses (`daemon.cpp` is the scheduler with function-pointer slots, `daemons.cpp` holds the callbacks `doctor`/`stomach`/`runners`/…). `playit()` (in `playit.cpp`, formerly `main.c`) loops over `command()` in `command.cpp`. The domain files are `fight`, `chase` (monster AI), `monsters`/`slime`, `things`/`pack`/`list` (items and the intrusive linked lists), `potions`/`scrolls`/`sticks`/`rings`/`armor`/`weapons`, level generation in `new_leve`/`rooms`/`passages`/`maze`, and endings/scores in `rip`.
- **Testing the UI headlessly**: `tests/ui/` drives `Screen`, `ScreenDisplay` and `ScreenInput` with fake `Terminal`s.

## Compile-time macros

CMake sets `MINROG` (unused) and `ROGUE_CHARSET=3`. `ROGUE_CHARSET=1` draws ASCII instead of Unicode. Remaining optional switches: `WIZARD` (debug commands, currently does not compile, see the notes in `docs/MODERNIZATION.md`), `DEBUG` and `ROGUE_DEBUG`.
