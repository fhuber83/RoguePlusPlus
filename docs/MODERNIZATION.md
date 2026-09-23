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

3. **Foundations.**
   - Add a CMake library target and a test executable (GoogleTest or doctest).
   - Add `core/` types: a `Random` class that replaces `rnd`/`ran`/`seed`, `Coord` with operators, and a `Dice` value type.
   - Add `enum class` item kinds and flags with a small bitflag helper.
4. **UI seam.**
   - Define `ui::Display` and `ui::Input`.
   - Route `msg`/`addmsg`/`more`, the map drawing (`mvaddch` in game files), the status line and the full-screen views through them.
   - Move `curses.cpp` and the drawing half of `io.cpp` into `ui/curses/`.
   - Delete the DOS-attribute emulation (`curses_dos.h`, CP437 translation) in favour of a clean `Glyph → cchar_t` mapping.
5. **Game state.** Gather the ~90 globals from `extern.cpp`/`init.cpp` into a `Game` context (player, level, monster list, floor items, RNG, scheduler, known-item tables, options). Free functions take or reach it explicitly, and globals are removed one group at a time.
6. **Entities.**
   - Split `union thing` into `Monster` and `Item`.
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

- `faststate` ("Fast Play") used to be toggled by Scroll Lock and is now always `FALSE`. Reintroduce it as a real option or key if wanted.
- `save_game()` prints "saving games is disabled" and `restore()` still contains the memory-dump code. Treat both as dead until phase 8.
- The terminal must be 80×25. `COLS == 40` paths still exist for the old 40-column mode.
