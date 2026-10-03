#!/usr/bin/env python3
"""Resume check: a game saved and restored plays on as if never saved.

    resume.py OUTDIR path/to/rogue++ --seeds 1 2 3 --save-after 100 [--keys 40] [--dump]

For each seed, run A plays keys P then K; run B plays P, saves with S, exits,
restores with -r and plays K. The map rows after each key of K must match, and
the save file must be gone after restoring. P holds only moves, runs, searches
and '>', so the save lands on a quiet turn; K is random play (replay.py's
keys). A seed whose rogue dies during P is reported and skipped.

While it plays, a line on stderr every --progress seconds (default 60, 0 for
none) says how far it is and when it should be done.
"""
import argparse
import os
import shutil
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from replay import Progress, alive, keys_for, sanitizer_logs, start, tmux  # noqa: E402

QUIET = set("hjklyubnHJKLYUBNs>") | {"Space"}


KEY_DELAY = 0.25
START = 0.8 + 2 * KEY_DELAY + 3  # start, the name, the curtain
SAVE = 4 * 0.8 + 1.0 + 1.5        # Escape Escape Space S, Enter, restoring


def keys_before_save(seed, n_save):
    """P: quiet keys, then two Spaces for a --More--."""
    return [k for k in keys_for(seed + 1000, n_save * 3) if k in QUIET][:n_save] + ["Space", "Space"]


def planned(seed, n_save, n_keys):
    """The seconds one seed's two runs pause for, as check() plays them."""
    one = START + (len(keys_before_save(seed, n_save)) + len(keys_for(seed, n_keys))) * KEY_DELAY
    return 2 * one + SAVE


def send(session, keys, delay=KEY_DELAY, capture=False, progress=None):
    caps = []
    for k in keys:
        tmux("send-keys", "-t", session, k, check=False)
        time.sleep(delay)
        if progress:
            progress.add(delay)
        if capture:
            rows = tmux("capture-pane", "-p", "-t", session, check=False).stdout.split("\n")
            caps.append("\n".join(rows[1:23]))  # the map: no message line, status or clock
    return caps


def check(outdir, binary, seed, n_save, n_keys, dump, progress=None):
    """0 when the runs match, 1 when not, None when there was no save to restore."""
    P = keys_before_save(seed, n_save)
    K = keys_for(seed, n_keys)
    played = 0.0  # what progress was given for this seed

    def add(seconds):
        nonlocal played
        played += seconds
        if progress:
            progress.add(seconds)

    def finish(result):
        if progress:
            progress.run_done(planned(seed, n_save, n_keys) - played)
        return result

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
        add(START)
        send(session, P, progress=progress)
        played += len(P) * KEY_DELAY
        if mode == "B":
            send(session, ["Escape", "Escape", "Space", "S"], delay=0.8)
            prompt = tmux("capture-pane", "-p", "-t", session, check=False).stdout.split("\n")[0]
            send(session, ["Enter"], delay=1.0)
            if alive(session):
                tmux("kill-session", "-t", session, check=False)
            if not os.path.exists(os.path.join(work, "rogue.sav")):
                print(f"seed {seed}: no save, the prompt was {prompt.strip()!r} (dead?)", flush=True)
                return finish(None)
            start(session, work, binary, "-r")
            time.sleep(1.5)
            add(SAVE)
        out[mode] = send(session, K, capture=True, progress=progress)
        played += len(K) * KEY_DELAY
        tmux("kill-session", "-t", session, check=False)
        if mode == "B" and os.path.exists(os.path.join(work, "rogue.sav")):
            print(f"seed {seed}: the save was not deleted", flush=True)
            return finish(1)
        if sanitizer_logs(work):
            print(f"seed {seed} {mode}: SANITIZER {sanitizer_logs(work)}", flush=True)
            return finish(1)
    diffs = [i for i, (a, b) in enumerate(zip(out["A"], out["B"])) if a != b]
    if diffs and dump:
        print(f"--- A, key {diffs[0]}\n{out['A'][diffs[0]]}\n--- B\n{out['B'][diffs[0]]}")
    print(f"seed {seed}: " + ("identical" if not diffs else f"{len(diffs)} differ {diffs[:6]}"), flush=True)
    return finish(int(bool(diffs)))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("outdir")
    ap.add_argument("binary")
    ap.add_argument("--seeds", nargs="+", type=int, required=True)
    ap.add_argument("--save-after", type=int, default=100, help="keys of P before saving")
    ap.add_argument("--keys", type=int, default=40, help="keys of K after restoring")
    ap.add_argument("--dump", action="store_true", help="print the first differing screens")
    ap.add_argument("--progress", type=float, default=60, metavar="SECONDS",
                    help="report progress and an ETA on stderr this often (0: never)")
    a = ap.parse_args()
    progress = Progress("resume", len(a.seeds),
                        sum(planned(s, a.save_after, a.keys) for s in a.seeds), a.progress, unit="seeds")
    results = [check(a.outdir, a.binary, s, a.save_after, a.keys, a.dump, progress) for s in a.seeds]
    progress.done()
    sys.exit(1 if any(results) else 0)


if __name__ == "__main__":
    main()
