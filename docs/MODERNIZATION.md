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

5. **Game state.** Globals moved into `rogue::Game` (`game/Game.hpp`), reached through `rogue::game()` the way `display()` is. `rogue.h` includes it after the legacy types it holds, so game files keep including only `rogue.h`. Each group of globals is deleted and the compiler finds every use, which leaves locals that shadow a global alone.
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
   - **5.4 The level.**
     - `game().level` (`rogue::Level`) holds `depth` (was `level`), `ntraps`, `no_food`, `rooms`, `passages`, the `map` and `flags` grids (were the allocated `_level`/`_flags`; `chat()`/`flat()` index them), and the `objects` and `monsters` lists (were `lvl_obj`/`mlist`). Its constructor makes every passage a dark, gone room, which replaces the 13-entry initializer.
     - `maxrow` is a constant next to `LINES`/`COLS`. It was always 23, and `setup()` no longer sets it.
     - Verified with the A/B replay, plus a descending replay on scratch builds of both trees patched so that `>` works anywhere. Three seeds went 6 to 24 levels deep, with mazes, traps and deaths along the way. Identical.
   - **5.5 Items.**
     - `game().items` (`rogue::Items`) holds what there is to find in this game and what the rogue knows about it. That covers the odds tables `s_magic`, `p_magic`, `r_magic`, `ws_magic` and `things`, the per-game looks (`s_names`, `p_colors`, `r_stones`, `ws_made`, `ws_type`), the `*_know` and `*_guess` tables with their storage (`guesses`, was `_guesses`) and `iguess`, the item `pool`/`pool_used` (were the allocated `_things`/`_t_alloc`) with `total`, and the weapon `group` counter. These keep their original names, since the prefixes are systematic.
     - The odds tables in `extern.cpp` are now `const` (`s_magic_base`, ..., `things_base`). `Items()` copies them, because `init_*()` accumulate the odds and add the stone value to the worth of rings. Before this change a second game in the same process would have accumulated the odds twice.
     - `f_damage` became `game().player.flytrap_damage`, next to `fung_hit`, which it grows with.
     - Verified with the A/B and descending replays: identical. `tests/game/GameTest.cpp` checks that the odds are copied per game and that the passages start dark and gone (moved from `StaticTablesTest`).
   - **5.6 Scheduler and RNG.**
     - `game().scheduler` holds the daemon and fuse slots that were `daemon.cpp`'s static `d_list`.
     - `game().random` is the game's `Random`, and `rogue::rng()` (now in `game/Game.hpp`) returns it, so `core/` no longer holds a global generator.
     - The `extern` section of `rogue.h` now lists only fixed tables, common strings and scratch buffers. Deleted the declared-but-undefined `is_me`.
     - Verified with the A/B and descending replays: identical.
   - **What stays outside `Game`, deliberately:** fixed tables (`monsters`, `w_names`, `a_names`, `a_class`, `a_chances`, `he_man`, help, the `*_base` odds, `e_levels`), common strings (`nullstr`, `it`, `you`, ...), scratch buffers (`prbuf`, `tbuf`, `ring_buf`, phase 9), per-algorithm file statics (`maze.cpp`, `passages.cpp`, `ch_ret`, `nh`, `slimy`, `things.cpp`'s paging, `env.cpp`'s parser, `rip.cpp`'s `file`, phase 7), and the clock state in `SIG2()`. `w_names[FLAME]` is still overwritten while a bolt flies (`sticks.cpp`).

6. **Entities.**
   - **6.1 Creature and Item.**
     - `union thing` (`THING`) is split into `rogue::Creature` (`entities/Creature.hpp`, a monster or the rogue's body, was `_t`) and `rogue::Item` (`entities/Item.hpp`, was `_o`). The members keep their `t_*`/`o_*` names but are real fields now, so the `#define t_pos _t._t_pos` accessor macros are gone. `o_charges` and `o_goldval` remain macro aliases of `o_ac`.
     - Every declaration and prototype got the type its role needs. Variables that held both kinds were split: `treas_room()`, `read_scroll()`, `add_pack()`'s monster loop and `door_open()`.
     - `new_item()` makes items and `new_creature()` makes monsters, from separate pools in `game().pool` that share one count. The original allocated both from one pool of `MAXITEMS` things, and level generation checks `total < MAXITEMS`, so the shared limit keeps dungeons identical. `discard()` has an overload for each.
     - `list_attach`/`list_detach`/`list_free` are templates in `rogue.h` for both kinds of list.
     - Verified with the A/B and descending replays: identical. `tests/game/GameTest.cpp` checks the shared pool limit.
   - **6.2 Flags.**
     - `t_flags` and `m_flags` are `CreatureFlags` (`rogue::Flags<CreatureFlag>`), and `o_flags` is `ItemFlags`. The legacy names (`ISBLIND`, `ISKNOW`, ...) remain as typed constants in `rogue.h`, so mixing a creature flag into an item, or the reverse, no longer compiles. `x |= F`, `x &= ~F` and `x & F` became `set`, `unset` and `test`, and `on()` uses `test`.
     - Two original quirks keep their bits. Scare monster scrolls remembered being picked up with the creature flag `ISFOUND`, the same bit as `ISEGO`; that is `ItemFlag::Found` now. The leprechaun has `ISGREED` (0x40) in its carry column, so it carries something 64% of the time and is not greedy.
     - Verified with the A/B and descending replays: identical. `StaticTablesTest` pins the monster flags.
   - **6.3 Item kinds.**
     - `o_type` is a `rogue::ItemKind` (`None`, `Potion`, ..., `Gold`, plus `Missile` for the bolt a wand of magic missile shoots). Kinds no longer share values with glyphs: `glyph_of(kind)` gives the CP437 glyph the map shows, and `kind_of_glyph(ch)` reads one back. The glyph constants (`POTION`, `GOLD`, ...) stay for the map, the help screen and `pick_up()`. Items are put on the map through `glyph_of()` in `new_leve.cpp`, `things.cpp` and `weapons.cpp`.
     - `get_item()` and `inventory()` take an `ItemFilter`: one kind, `ItemFilter::all()` or `ItemFilter::callable()`, which replace the `0` and `CALLABLE` (-1) sentinels.
     - Switches over a kind that the original left without a default now end in `otherwise: break;`, so the new enum values are handled explicitly and `-Wswitch` stays quiet.
     - Verified with the A/B and descending replays, plus a wizard replay on scratch `WIZARD` builds of both trees. It creates potions, scrolls, wands, rings, weapons, armor, food, gold and the amulet, then quaffs, reads, zaps, puts on, wears, wields, throws, eats, drops and names them. Identical. `tests/entities/ItemTest.cpp` covers the glyph mapping and the filter.
   - **6.4 Lists.**
     - The intrusive `l_next`/`l_prev` links are gone. `rogue::List<T>` (`entities/List.hpp`, over `std::list<T *>`) holds the level's `monsters` and `objects` and every creature's `t_pack`. `attach()`/`detach()`/`free_list()` stay as macros over it, and the `next()`/`prev()` macros are gone.
     - Walks keep their original shape: `for (tp = list.first(); tp != NULL; tp = list.after(tp))`. `after()` returns null for an entry that is no longer in the list, just as a detached node's cleared `l_next` did. `runners()` relies on that: when a nymph or leprechaun vanishes during its move, the other monsters skip theirs that turn, as in the original.
     - **Deviation from the plan:** the pool in `game().pool` still owns creatures and items, instead of `std::unique_ptr`. The original reads things after freeing them, and that only works because a freed slot keeps its contents until it is reused: the nymph's theft message after `discard()`, `t_dest` pointing into gold the rogue picked up, and a vanished thief that is hasted or flying moving again in `runners()`. With heap ownership each of these is a use-after-free. A pool slot is a stable identity, so saving games (phase 8) can refer to slots. Moving to owning containers needs those reads fixed first (phase 7).
     - Verified with the A/B, descending and wizard item replays: identical. `tests/entities/ListTest.cpp` covers the list, including a walk that detaches its current entry.

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
5. **Game state** (*done*, see above). Gather the ~90 globals from `extern.cpp`/`init.cpp` into a `Game` context (player, level, monster list, floor items, RNG, scheduler, known-item tables, options). Free functions take or reach it explicitly, and globals are removed one group at a time. Steps:
   1. *Done:* options (see above).
   2. *Done:* messages and command state (see above).
   3. *Done:* the player (see above).
   4. *Done:* the level (see above).
   5. *Done:* items (see above).
   6. *Done:* the scheduler (`daemon.cpp` slots) and the RNG.
   Algorithm scratch state (`maze.cpp`, `passages.cpp`, `ch_ret`, ...) and fixed tables stay where they are until phase 7.
6. **Entities** (*done*, see above).
   - *Done:* split `union thing` into `Creature` (monster or player) and `Item`.
   - *Done:* creature/object flags become `rogue::Flags`.
   - *Done:* item kinds become an `enum class` with a separate glyph mapping.
   - *Done:* replace the intrusive `l_next`/`l_prev` lists (`list.cpp`) with standard containers. Ownership stays with the pool, whose slots are the stable IDs (see 6.4 for why not `std::unique_ptr` yet).
   - *Done:* replace the `#define t_pos _t._t_pos` accessor macros with members.
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

- Code reads creatures and items after `discard()`, which is safe only because pool slots keep their contents until reused. Before creatures and items can move to `std::unique_ptr` ownership, these reads have to go: `inv_name(steal)` after `discard(steal)` in `attack()` (nymph), `t_dest` left pointing at gold that `pick_up()` discards, and `runners()` chasing a monster again after `attack()` removed it (hasted or flying thieves). Fixing them changes behaviour in those corners, so do it as its own step.
- `score()` writes `sc_name[38]` to `rogue.scr` with uninitialized bytes after the name. Harmless, but compare score files by the name up to its NUL. Phase 8 replaces the format.

- `faststate` ("Fast Play") used to be toggled by Scroll Lock and is now always `FALSE`. Reintroduce it as a real option or key if wanted.
- `save_game()` prints "saving games is disabled" and `restore()` exits with a message. Phase 8 brings real saving.
- `WIZARD` builds do not compile: `CTRL(D)` in `command.cpp` should be `CTRL('D')`, `rogue.h` defines `bool wizard;` in the header (it should be `extern`, while `extern.cpp` defines it only under `WIZARD`), and `create_obj()` passes a `short *` and `stdscr` to `get_num(int *)`.
- The terminal must be 80×25. `COLS == 40` paths still exist for the old 40-column mode.
