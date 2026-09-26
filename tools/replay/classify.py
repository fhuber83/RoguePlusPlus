#!/usr/bin/env python3
"""Sort the differences of a replay.py run into timing noise and divergence.

    classify.py OUTDIR REF NAME... --seeds 1 2 3

Compares OUTDIR/NAME-SEED against OUTDIR/REF-SEED capture by capture, as text
(colours stripped). Each differing capture is reported with the screen rows
that differ and whether the next capture matches again. A clock tick is only
row 24; an animation caught mid-frame (curtain, level wipe) is a short run
that recovers. A real divergence persists, and shows in the final screens,
which are compared last. The exit status is 1 if any final screen differs.
"""
import argparse
import os
import re
import sys


def captures(path):
    parts = re.split(r"^=== \d+ .*\n", open(path).read(), flags=re.M)[1:]
    return [re.sub(r"\x1b\[[0-9;]*m", "", c).split("\n") for c in parts]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("outdir")
    ap.add_argument("ref")
    ap.add_argument("names", nargs="+")
    ap.add_argument("--seeds", nargs="+", type=int, required=True)
    ap.add_argument("--quiet", action="store_true", help="only the final screens")
    a = ap.parse_args()
    bad = 0
    for seed in a.seeds:
        paths = [f"{a.outdir}/{n}-{seed}/caps.txt" for n in [a.ref] + a.names]
        missing = [p for p in paths if not os.path.exists(p)]
        if missing:
            print(f"seed {seed}: no captures in {', '.join(missing)}")
            bad += 1
            continue
        ref = captures(paths[0])
        for name in a.names:
            other = captures(f"{a.outdir}/{name}-{seed}/caps.txt")
            n = min(len(ref), len(other))
            for i in range(n):
                if ref[i] == other[i] or a.quiet:
                    continue
                rows = [r for r in range(max(len(ref[i]), len(other[i])))
                        if ref[i][r:r + 1] != other[i][r:r + 1]]
                kind = "clock" if rows == [24] else "colour only" if not rows else f"rows {rows}"
                after = "recovers" if i + 1 >= n or ref[i + 1] == other[i + 1] else "persists"
                print(f"seed {seed} {name} capture {i}: {kind}, {after}")
            # The final map and status, without the clock row
            same = len(ref) == len(other) and (n == 0 or ref[n - 1][:24] == other[n - 1][:24])
            print(f"seed {seed} {name}: final screen {'identical' if same else 'DIFFERS'}")
            bad += not same
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
