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

While it plays, a line on stderr every --progress seconds (default 60, 0 for
none) says how far it is and when it should be done.
"""
import argparse
import os
import random
import shutil
import subprocess
import sys
import threading
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


SETUP = 3.8  # seconds before a run's first key: the start and the curtain


def duration(seconds):
    seconds = int(seconds)
    return f"{seconds // 3600}h{seconds // 60 % 60:02d}m" if seconds >= 3600 else f"{seconds // 60}m{seconds % 60:02d}s"


class Progress:
    """How far the runs are, in seconds of the pauses they plan (the delay
    after each key, the start), which the measured rate turns into an ETA.
    Every interval seconds a line goes to stderr; done() stops it."""

    def __init__(self, label, runs, planned, interval, unit="runs"):
        self.label, self.runs, self.planned, self.interval, self.unit = label, runs, planned, interval, unit
        self.played = 0.0
        self.finished = 0
        self.start = time.monotonic()
        self.lock = threading.Lock()
        self.stop = threading.Event()
        if interval > 0:
            threading.Thread(target=self._report, daemon=True).start()

    def add(self, seconds):
        """A run played this much of its plan."""
        with self.lock:
            self.played += seconds

    def run_done(self, unplayed=0.0):
        """A run ended; what it planned but didn't play (the rogue died) is
        taken off the total."""
        with self.lock:
            self.finished += 1
            self.planned -= unplayed

    def line(self):
        with self.lock:
            played, planned, finished = self.played, self.planned, self.finished
        elapsed = time.monotonic() - self.start
        text = (f"{self.label}: {100 * played / max(planned, 1e-9):.0f}%, "
                f"{finished}/{self.runs} {self.unit} done, {duration(elapsed)} elapsed")
        if played > 0 and elapsed > 0:
            left = (planned - played) * elapsed / played
            text += f", about {duration(left)} left (done at {time.strftime('%H:%M', time.localtime(time.time() + left))})"
        return text

    def _report(self):
        while not self.stop.wait(self.interval):
            print(self.line(), file=sys.stderr, flush=True)

    def done(self):
        self.stop.set()
        if self.interval > 0:
            print(f"{self.label}: done in {duration(time.monotonic() - self.start)}", file=sys.stderr, flush=True)


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


def play(outdir, name, binary, seed, keys, delay, progress=None):
    work = os.path.abspath(os.path.join(outdir, f"{name}-{seed}"))
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    session = f"replay-{os.getpid()}-{name}-{seed}"
    start(session, work, binary, f"-d {seed}")
    time.sleep(0.8)
    tmux("send-keys", "-t", session, "Tester", "Enter")
    time.sleep(3)  # the curtain animation
    if progress:
        progress.add(SETUP)
    caps = []
    for k in keys:
        if not alive(session):
            break
        tmux("send-keys", "-t", session, k, check=False)
        time.sleep(delay)
        caps.append(tmux("capture-pane", "-p", "-e", "-t", session, check=False).stdout)
        if progress:
            progress.add(delay)
    tmux("kill-session", "-t", session, check=False)
    if progress:
        progress.run_done((len(keys) - len(caps)) * delay)
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
    ap.add_argument("--progress", type=float, default=60, metavar="SECONDS",
                    help="report progress and an ETA on stderr this often (0: never)")
    a = ap.parse_args()
    builds = [b.split("=", 1) for b in a.builds]
    for b in builds:
        if len(b) != 2 or not os.access(b[1], os.X_OK):
            ap.error(f"not NAME=BINARY with an executable BINARY: {'='.join(b)!r}")

    keys = {seed: keys_for(seed, a.keys, a.dive) for seed in a.seeds}
    runs = [(seed, name, binary) for seed in a.seeds for name, binary in builds]
    progress = Progress("replay", len(runs),
                        sum(SETUP + len(keys[seed]) * a.delay for seed, _, _ in runs), a.progress)
    with ThreadPoolExecutor(a.jobs) as ex:
        futures = {(seed, name): ex.submit(play, a.outdir, name, binary, seed,
                                           keys[seed], a.delay, progress)
                   for seed, name, binary in runs}
    results = {k: f.result() for k, f in futures.items()}
    progress.done()

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
