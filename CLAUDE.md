# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

RoguePlusPlus is a C++ port of the classic PC game "Rogue", based on the PC Rogue 1.48 sources (A.I. Design / Epyx, 1983–84) as ported to Linux/ncurses. The code now compiles as **C++23**, but it is still structured like the original C: free functions, global state, and `union thing`. It is being modernized incrementally into separate modules. The plan, the decisions already made and the progress so far are in **`docs/MODERNIZATION.md`**. Read it before any structural change, and update its "Done" section when a phase step lands.

## Build & run

Requirements: CMake ≥ 4.0, a C++23 compiler (GCC 15 is used), pkg-config, and ncursesw.

```sh
cmake -S . -B build
cmake --build build
./build/rogue++            # new game
./build/rogue++ -s         # show scores
```

Sources are picked up by `FILE(GLOB ... src/*.cpp CONFIGURE_DEPENDS)`. The build uses `-Wall -Wextra -Wpedantic` and **must stay warning-free**. There are no tests yet.

Smoke test without a real terminal: `tmux new-session -d -s rg -x 80 -y 25 ./build/rogue++`, then `tmux send-keys -t rg ...` and `tmux capture-pane -p -t rg`. The game needs an 80×25 screen. After the name prompt, wait about 2.5 s for the curtain animation to finish.

At runtime the game reads `rogue.opt` (options) and writes `rogue.scr` (scores) in the current working directory. Saving is disabled: `save_game()` is a stub and `restore()` is dead memory-dump code.

## Conventions

- **`//@` and `/*@` comments** mark changes made by the Linux port and this project. Everything else is original 1980s code.
- Keep string handling `const`-correct. Buffers the game really writes to (`prbuf`, `f_damage`, `s_names`, `_guesses`, …) are `char[]`. Everything else is `const char *`.

## Architecture (current, pre-modularization)

- **Headers**:
  - `extern.h`: libc includes, POSIX feature macros, and libc "overrides": `#define srand md_srand`, `#define access(f) access(f, F_OK)`, `setmem`/`bcopy`. Remember these when a libc call behaves unexpectedly.
  - `rogue.h`: game constants, structs, globals, prototypes, plus accessor macros like `#define t_pos _t._t_pos` over `union thing` (THING).
  - `extern.cpp`/`init.cpp`: define most of the globals.
- **Curses layer (two-sided)**: `curses.cpp` reimplements the original DOS screen API on top of ncurses. It translates DOS attributes and CP437 codes to ncurses colours and Unicode. It is the only file that includes the system `<curses.h>`, together with `curses_common.h` and the private `curses_dos.h`. Game files include the local `"curses.h"` instead. That header maps `move`, `clear`, `inch`, `standout`, etc. onto `cur_*` functions and turns `stdscr`/`hw` into `NULL`. **Never include the system `<curses.h>` in game files, and never include the local `curses.h` in `curses.cpp`.**
- **Machine layer**: `mach_dep.cpp` holds time, sleep, the seed source (`md_srand`), `readchar`, `newmem`, `fatal`/`md_exit`, and the credits screen.
- **Game loop**: `main.cpp` sets up the game with `init_*()` → `new_level()`, then starts daemons and fuses (`daemon.cpp` is the scheduler with function-pointer slots, `daemons.cpp` holds the callbacks `doctor`/`stomach`/`runners`/…). `playit()` loops over `command()` in `command.cpp`. The domain files are `fight`, `chase` (monster AI), `monsters`/`slime`, `things`/`pack`/`list` (items and the intrusive linked lists), `potions`/`scrolls`/`sticks`/`rings`/`armor`/`weapons`, level generation in `new_leve`/`rooms`/`passages`/`maze`, and endings/scores in `rip`.
- **Output from game logic** goes straight to the screen: `msg()`/`addmsg()` in `io.cpp`, and `mvaddch` in map code. Phase 4 of the roadmap puts this behind a `Display` interface.

## Compile-time macros

CMake sets `MINROG` and `ROGUE_CHARSET=3` (1=ASCII, 2=CP437, 3=UNICODE; UNICODE falls back to ASCII without wide-char curses). Remaining optional switches: `WIZARD` (debug commands), `DEBUG`, `ROGUE_DEBUG`, `ROGUE_COLUMNS` (default 80), and `ROGUE_SCR_TYPE`.
