#!/usr/bin/env python3
"""A/B replay: play the same seeded keys on several rogue++ builds in tmux and
compare the screens after every key.

    replay.py OUTDIR base=path/to/rogue++ new=path/to/rogue++ ... \\
        --seeds 1 2 3 --keys 600 [--dive] [--delay 0.2] [--jobs 12]

The first build is the reference. Each run plays in OUTDIR/NAME-SEED, which
gets its captures (caps.txt) and any AddressSanitizer/UBSan logs. The summary
lists per seed how many captures differ and any sanitizer reports; the exit
status is 1 if anything differs. Use classify.py to tell timing noise (clock
ticks, animations) from real divergence.
"""
import argparse
import os
import random
import shutil
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor

LETTERS = "abcdefghijklmn"
DIRS = "hjklyubn"


def keys_for(seed, count, dive=False):
    """The keys a replay types for seed: the same on every build."""
    r = random.Random(seed * 7919)
    if dive:
        return [r.choice([">", ">", "Space", "s", "h", "j", "k", "l", "q", "a", "e", "a"])
                for _ in range(count)]
    out = []
    while len(out) < count:
        x = r.random()
        if x < 0.40:
            out.append(r.choice(DIRS))
        elif x < 0.46:
            out.append(r.choice(DIRS.upper()))
        elif x < 0.50:
            out.append("s")
        elif x < 0.54:
            out.append(">")
        elif x < 0.72:
            cmd = r.choice("qqrreewWTPRdcD")
            out.append(cmd)
            if cmd in "PR":
                out += [r.choice(LETTERS), r.choice("lr")]
            elif cmd == "c":
                out += [r.choice(LETTERS), "x", "Enter"]
            elif cmd != "D":
                out.append(r.choice(LETTERS))
        elif x < 0.80:
            out += [r.choice("tz"), r.choice(DIRS), r.choice(LETTERS)]
        elif x < 0.83:
            out.append("a")
        elif x < 0.85:
            out += [str(r.randint(2, 9)), r.choice("s" + DIRS)]
        elif x < 0.87:
            out.append("i")
        elif x < 0.89:
            out.append("Escape")
        else:
            out.append("Space")
    return out


def tmux(*args, check=True):
    return subprocess.run(["tmux", *args], capture_output=True, text=True, check=check)


def alive(session):
    return tmux("has-session", "-t", session, check=False).returncode == 0


def start(session, work, binary, args):
    """Start binary in an 80x25 tmux session, sanitizer logs going to work."""
    env = (f"ASAN_OPTIONS=log_path={work}/asan:detect_leaks=0 "
           f"UBSAN_OPTIONS=log_path={work}/ubsan:print_stacktrace=1")
    # The trailing sleep keeps the last screen (tombstone, scores) capturable
    tmux("new-session", "-d", "-s", session, "-x", "80", "-y", "25", "-c", work,
         f"env {env} {os.path.abspath(binary)} {args}; sleep 3")


def sanitizer_logs(work):
    return [f for f in os.listdir(work) if f.startswith(("asan", "ubsan"))]


def play(outdir, name, binary, seed, keys, delay):
    work = os.path.abspath(os.path.join(outdir, f"{name}-{seed}"))
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    session = f"replay-{os.getpid()}-{name}-{seed}"
    start(session, work, binary, f"-d {seed}")
    time.sleep(0.8)
    tmux("send-keys", "-t", session, "Tester", "Enter")
    time.sleep(3)  # the curtain animation
    caps = []
    for k in keys:
        if not alive(session):
            break
        tmux("send-keys", "-t", session, k, check=False)
        time.sleep(delay)
        caps.append(tmux("capture-pane", "-p", "-e", "-t", session, check=False).stdout)
    tmux("kill-session", "-t", session, check=False)
    with open(os.path.join(work, "caps.txt"), "w") as f:
        for i, c in enumerate(caps):
            f.write(f"=== {i} {keys[i]}\n{c}")
    return caps


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("outdir")
    ap.add_argument("builds", nargs="+", help="NAME=BINARY, the first is the reference")
    ap.add_argument("--seeds", nargs="+", type=int, required=True)
    ap.add_argument("--keys", type=int, default=300)
    ap.add_argument("--dive", action="store_true", help="mostly '>': go deep fast (needs the stairs patch)")
    ap.add_argument("--delay", type=float, default=0.2, help="seconds between keys")
    ap.add_argument("--jobs", type=int, default=6, help="runs at once")
    a = ap.parse_args()
    builds = [b.split("=", 1) for b in a.builds]
    for b in builds:
        if len(b) != 2 or not os.access(b[1], os.X_OK):
            ap.error(f"not NAME=BINARY with an executable BINARY: {'='.join(b)!r}")

    with ThreadPoolExecutor(a.jobs) as ex:
        futures = {(seed, name): ex.submit(play, a.outdir, name, binary, seed,
                                           keys_for(seed, a.keys, a.dive), a.delay)
                   for seed in a.seeds for name, binary in builds}
    results = {k: f.result() for k, f in futures.items()}

    bad = 0
    for seed in a.seeds:
        ref = results[(seed, builds[0][0])]
        line = [f"seed {seed}: {len(ref)} captures"]
        for name, _ in builds[1:]:
            other = results[(seed, name)]
            diffs = [i for i in range(max(len(ref), len(other)))
                     if i >= len(ref) or i >= len(other) or ref[i] != other[i]]
            line.append(f"{name}: " + ("identical" if not diffs else f"{len(diffs)} differ, first {diffs[:5]}"))
            bad += bool(diffs)
        for name, _ in builds:
            logs = sanitizer_logs(os.path.join(a.outdir, f"{name}-{seed}"))
            if logs:
                line.append(f"{name}: SANITIZER {logs}")
                bad += 1
        print("; ".join(line), flush=True)
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
