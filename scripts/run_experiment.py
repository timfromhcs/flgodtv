#!/usr/bin/env python3
"""Reproducible MPE experiment runner (Phase 25).
Runs a scenario N times via the headless CLI, captures telemetry + hashes +
environment, and writes evidence/experiments/<experiment_id>.json.
Usage: run_experiment.py <scenario_path> [--ticks N] [--repeats R] [--id NAME]
"""
import hashlib
import json
import os
import platform
import subprocess
import sys
import time
from datetime import datetime, timezone

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
BUILD = os.path.join(ROOT, "build", "ninja-release")
FLGOD = os.path.join(BUILD, "flgod.exe" if os.name == "nt" else "flgod")
EVEXP = os.path.join(ROOT, "evidence", "experiments")


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def main():
    if len(sys.argv) < 2 or sys.argv[1] in ("-h", "--help"):
        print(__doc__)
        return 2
    scenario = sys.argv[1]
    ticks = 0
    repeats = 2
    exp_id = None
    args = sys.argv[2:]
    i = 0
    while i < len(args):
        if args[i] == "--ticks":
            ticks = int(args[i + 1]); i += 2
        elif args[i] == "--repeats":
            repeats = int(args[i + 1]); i += 2
        elif args[i] == "--id":
            exp_id = args[i + 1]; i += 2
        else:
            print(f"unknown arg {args[i]}"); return 2
    if not os.path.isfile(FLGOD):
        print(f"missing binary {FLGOD}"); return 1
    if exp_id is None:
        base = os.path.splitext(os.path.basename(scenario))[0]
        exp_id = f"{base}_{datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')}"
    os.makedirs(EVEXP, exist_ok=True)
    runs = []
    for r in range(repeats):
        tel_path = os.path.join(EVEXP, f".{exp_id}_run{r}.json")
        cmd = [FLGOD, "--scenario", scenario, "--telemetry-out", tel_path]
        if ticks:
            cmd.append(str(ticks))
        t0 = time.perf_counter()
        res = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
        dt = time.perf_counter() - t0
        if res.returncode != 0:
            print(f"run {r} FAILED: {res.stderr.strip()}")
            return 1
        tel = json.load(open(tel_path))
        os.remove(tel_path)
        runs.append({"run": r, "wall_seconds": dt, "stdout_tail": res.stdout.strip().splitlines()[-1],
                     "telemetry": tel["telemetry"], "state_hash": tel["state_hash"]})
    hashes = {run["state_hash"] for run in runs}
    record = {
        "experiment_id": exp_id,
        "scenario": scenario,
        "ticks": ticks or runs[0]["telemetry"]["tick"],
        "repeats": repeats,
        "deterministic": len(hashes) == 1,
        "state_hash": runs[0]["state_hash"],
        "runs": runs,
        "environment": {
            "os": platform.platform(),
            "cpu": platform.processor() or platform.machine(),
            "flgod_sha256": sha256_file(FLGOD),
        },
        "timestamp": datetime.now(timezone.utc).isoformat(),
    }
    out = os.path.join(EVEXP, f"{exp_id}.json")
    json.dump(record, open(out, "w"), indent=2)
    print(f"experiment {exp_id}: deterministic={record['deterministic']} "
          f"hash={record['state_hash']} -> {out}")
    return 0 if record["deterministic"] else 1


if __name__ == "__main__":
    sys.exit(main())
