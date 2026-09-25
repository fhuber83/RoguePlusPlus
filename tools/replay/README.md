# Replay tools

Scripts that check a change doesn't alter the game. They play the same seeded
keys on two or more builds in tmux (80×25) and compare the screens after every
key. These are the A/B, descending, dive and resume replays that
`docs/MODERNIZATION.md` refers to. They need Python 3, tmux and bash.

```sh
# 1. Build main, the working tree, and the working tree with ASan/UBSan,
#    all with the stairs check patched out so '>' works anywhere
tools/replay/make-trees.sh /tmp/rg

# 2. Random play (moves, fights, items, descending), 12 seeds
tools/replay/replay.py /tmp/rg/out base=/tmp/rg/base/build/rogue++ \
    new=/tmp/rg/new/build/rogue++ asan=/tmp/rg/asan/build/rogue++ \
    --seeds $(seq 1 12) --keys 600 --jobs 18

#    Deep levels (mazes start below level 10): mostly '>'
tools/replay/replay.py /tmp/rg/dive base=... new=... --seeds $(seq 21 28) --keys 300 --dive --delay 0.5

# 3. Sort the differences into timing noise and real divergence
tools/replay/classify.py /tmp/rg/out base new asan --seeds $(seq 1 12)

# 4. Save and restore: resuming must play on as if never saved
tools/replay/resume.py /tmp/rg/resume /tmp/rg/new/build/rogue++ --seeds $(seq 1 12) --save-after 100
```

## Reading the results

- `replay.py` reports how many captures differ from the first build, and any
  AddressSanitizer or UBSan report. The logs are in `OUTDIR/NAME-SEED/`, next to
  `caps.txt`, which holds every capture with the key that led to it.
- The game shows a clock, and the curtain, level wipe and tombstone
  animations run in real time. A capture can therefore differ without the game
  differing: a clock tick, or an animation caught at another frame, most often
  at startup and in the slower ASan build. `classify.py` names these (a clock
  row, or a short run that recovers). A real divergence persists, and the final
  screens differ, since both builds roll the same numbers from the same seed.
- If a difference is in doubt, rerun with a longer `--delay` or fewer
  `--jobs`, and replay the reference against itself (`base=... base2=...`)
  to see how much noise there is.
- `resume.py` skips seeds where the rogue dies before the save point, and
  prints the prompt it found instead of the save prompt. `--dump` prints the
  first pair of differing screens.
- A game that dies ends with the tombstone and the score list, and `rogue.scr`
  is written in the run's directory.
