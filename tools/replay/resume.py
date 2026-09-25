#!/usr/bin/env python3
"""Resume check: a game saved and restored plays on as if never saved.

    resume.py OUTDIR path/to/rogue++ --seeds 1 2 3 --save-after 100 [--keys 40] [--dump]

For each seed, run A plays keys P then K; run B plays P, saves with S, exits,
restores with -r and plays K. The map rows after each key of K must match, and
the save file must be gone after restoring. P holds only moves, runs, searches
and '>', so the save lands on a quiet turn; K is random play (replay.py's
keys). A seed whose rogue dies during P is reported and skipped.
"""
import argparse
import os
import shutil
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from replay import alive, keys_for, sanitizer_logs, start, tmux  # noqa: E402

QUIET = set("hjklyubnHJKLYUBNs>") | {"Space"}


def send(session, keys, delay=0.25, capture=False):
    caps = []
    for k in keys:
        tmux("send-keys", "-t", session, k, check=False)
        time.sleep(delay)
        if capture:
            rows = tmux("capture-pane", "-p", "-t", session, check=False).stdout.split("\n")
            caps.append("\n".join(rows[1:23]))  # the map: no message line, status or clock
    return caps


def check(outdir, binary, seed, n_save, n_keys, dump):
    """0 when the runs match, 1 when not, None when there was no save to restore."""
    P = [k for k in keys_for(seed + 1000, n_save * 3) if k in QUIET][:n_save] + ["Space", "Space"]
    K = keys_for(seed, n_keys)
    out = {}
    for mode in ("A", "B"):
        work = os.path.abspath(os.path.join(outdir, f"{mode}-{seed}"))
        shutil.rmtree(work, ignore_errors=True)
        os.makedirs(work)
        session = f"resume-{os.getpid()}-{mode}-{seed}"
        start(session, work, binary, f"-d {seed}")
        time.sleep(0.8)
        send(session, ["Tester", "Enter"])
        time.sleep(3)
        send(session, P)
        if mode == "B":
            send(session, ["Escape", "Escape", "Space", "S"], delay=0.8)
            prompt = tmux("capture-pane", "-p", "-t", session, check=False).stdout.split("\n")[0]
            send(session, ["Enter"], delay=1.0)
            if alive(session):
                tmux("kill-session", "-t", session, check=False)
            if not os.path.exists(os.path.join(work, "rogue.sav")):
                print(f"seed {seed}: no save, the prompt was {prompt.strip()!r} (dead?)")
                return None
            start(session, work, binary, "-r")
            time.sleep(1.5)
        out[mode] = send(session, K, capture=True)
        tmux("kill-session", "-t", session, check=False)
        if mode == "B" and os.path.exists(os.path.join(work, "rogue.sav")):
            print(f"seed {seed}: the save was not deleted")
            return 1
        if sanitizer_logs(work):
            print(f"seed {seed} {mode}: SANITIZER {sanitizer_logs(work)}")
            return 1
    diffs = [i for i, (a, b) in enumerate(zip(out["A"], out["B"])) if a != b]
    if diffs and dump:
        print(f"--- A, key {diffs[0]}\n{out['A'][diffs[0]]}\n--- B\n{out['B'][diffs[0]]}")
    print(f"seed {seed}: " + ("identical" if not diffs else f"{len(diffs)} differ {diffs[:6]}"), flush=True)
    return int(bool(diffs))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("outdir")
    ap.add_argument("binary")
    ap.add_argument("--seeds", nargs="+", type=int, required=True)
    ap.add_argument("--save-after", type=int, default=100, help="keys of P before saving")
    ap.add_argument("--keys", type=int, default=40, help="keys of K after restoring")
    ap.add_argument("--dump", action="store_true", help="print the first differing screens")
    a = ap.parse_args()
    results = [check(a.outdir, a.binary, s, a.save_after, a.keys, a.dump) for s in a.seeds]
    sys.exit(1 if any(results) else 0)


if __name__ == "__main__":
    main()
