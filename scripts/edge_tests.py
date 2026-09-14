#!/usr/bin/env python3
"""Independent black-box edge harness (Phase 27).
Treats the built application as a black box: CLI exit codes, malformed inputs,
missing resources, corrupt data, scale, and determinism. No internal APIs.
Usage: edge_tests.py [--build-dir DIR]  (exit 0 = all pass)
"""
import json
import os
import subprocess
import sys
import tempfile
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
BUILD = os.path.join(ROOT, "build", "ninja-release")
FLGOD = os.path.join(BUILD, "flgod.exe" if os.name == "nt" else "flgod")

PASS = []
FAIL = []


def check(name, cond, detail=""):
    (PASS if cond else FAIL).append(name)
    print(("  [PASS] " if cond else "  [FAIL] ") + name + (f" ({detail})" if detail else ""))


def run(*args, timeout=120):
    try:
        r = subprocess.run([FLGOD, *args], capture_output=True, text=True, cwd=ROOT, timeout=timeout)
        return r.returncode, (r.stdout + r.stderr)[-500:]
    except subprocess.TimeoutExpired:
        return None, "TIMEOUT"


def main():
    for a in sys.argv[1:]:
        if a.startswith("--build-dir="):
            global BUILD, FLGOD
            BUILD = a.split("=", 1)[1]
            FLGOD = os.path.join(BUILD, "flgod.exe" if os.name == "nt" else "flgod")
    print("[EDGE] black-box harness against " + FLGOD)
    if not os.path.isfile(FLGOD):
        print("[EDGE] binary missing"); return 1

    rc, _ = run("--version")
    check("version exits 0", rc == 0)
    rc, _ = run("--help")
    check("help exits 0", rc == 0)
    rc, _ = run("--no-such-flag")
    check("unknown flag exits non-zero", rc != 0)
    rc, _ = run("--scenario")
    check("scenario without path exits non-zero", rc != 0)
    rc, out = run("--scenario", "scenarios/nope.json")
    check("unknown scenario exits non-zero", rc != 0, out.strip().splitlines()[-1][:80] if out.strip() else "")

    with tempfile.TemporaryDirectory() as tmp:
        bad = os.path.join(tmp, "bad.json")
        open(bad, "w").write("{not json")
        rc, _ = run("--scenario", bad)
        check("malformed scenario exits non-zero", rc != 0)

        empty_pop = os.path.join(tmp, "empty.json")
        json.dump({"name": "e", "scenario_version": 1, "master_seed": 1, "populations": []}, open(empty_pop, "w"))
        rc, _ = run("--scenario", empty_pop)
        check("empty populations exits non-zero", rc != 0)

        zero = os.path.join(tmp, "zero.json")
        json.dump({"name": "z", "scenario_version": 1, "master_seed": 1, "ticks": 20,
                   "populations": [{"archetype": "ant", "count": 0}]}, open(zero, "w"))
        rc, out = run("--scenario", zero, "--archetypes", os.path.join(ROOT, "scenarios/archetypes"))
        check("zero entities exits 0", rc == 0, out.strip().splitlines()[-1][:100] if out.strip() else "")

        big = os.path.join(tmp, "big.json")
        json.dump({"name": "big", "scenario_version": 1, "master_seed": 7, "ticks": 30,
                   "populations": [{"archetype": "robot", "count": 200}]}, open(big, "w"))
        t0 = time.perf_counter()
        rc, out = run("--scenario", big, "--archetypes", os.path.join(ROOT, "scenarios/archetypes"), timeout=300)
        dt = time.perf_counter() - t0
        check("200 entities complete", rc == 0, f"{dt:.1f}s")

    # Determinism: same scenario twice => identical hash line.
    _, out1 = run("--scenario", "scenarios/03_ant_colony.json")
    _, out2 = run("--scenario", "scenarios/03_ant_colony.json")
    h1 = out1.strip().splitlines()[-1] if out1.strip() else ""
    h2 = out2.strip().splitlines()[-1] if out2.strip() else ""
    check("repeat runs bit-identical", h1 == h2 and "hash=" in h1, h1[-40:] if h1 else "empty")

    rc, _ = run("--simulate", "60")
    check("headless simulate exits 0", rc == 0)
    rc, _ = run("--self-test")
    check("self-test exits 0", rc == 0)

    print(f"[EDGE] {len(PASS)} passed, {len(FAIL)} failed")
    return 0 if not FAIL else 1


if __name__ == "__main__":
    sys.exit(main())
