"""Subprocess client for the compiled flgod headless binary."""
import json
import os
import subprocess

__version__ = "0.1.0"

_CANDIDATES = [
    os.path.join("build", "ninja-release", "flgod.exe"),
    os.path.join("build", "ninja-release", "flgod"),
    os.path.join("build", "flgod.exe"),
    os.path.join("build", "flgod"),
]


def find_binary(repo_root="."):
    """Locate the compiled flgod binary; raise FileNotFoundError if absent."""
    for c in _CANDIDATES:
        p = os.path.join(repo_root, c)
        if os.path.isfile(p):
            return os.path.abspath(p)
    raise FileNotFoundError(
        "flgod binary not found; build first (cmake --preset windows-ninja-release)")


def run_cli(*args, repo_root="."):
    """Run flgod with argv; return CompletedProcess (no exception on failure)."""
    binary = find_binary(repo_root)
    return subprocess.run([binary, *args], capture_output=True, text=True, cwd=repo_root)


def run_scenario(scenario, ticks=0, archetypes=None, repo_root=".", timeout=300):
    """Execute a scenario; return dict with returncode, telemetry, state_hash, stdout."""
    args = ["--scenario", scenario]
    if archetypes:
        args += ["--archetypes", archetypes]
    if ticks:
        args.append(str(ticks))
    tel_path = os.path.join(repo_root, ".flgodtv_tel.json")
    args += ["--telemetry-out", tel_path]
    try:
        proc = run_cli(*args, repo_root=repo_root)
        telemetry = None
        if os.path.isfile(tel_path):
            with open(tel_path) as f:
                telemetry = json.load(f)
    finally:
        if os.path.isfile(tel_path):
            os.remove(tel_path)
    return {
        "returncode": proc.returncode,
        "stdout": proc.stdout,
        "stderr": proc.stderr,
        "telemetry": telemetry["telemetry"] if telemetry else None,
        "state_hash": telemetry["state_hash"] if telemetry else None,
    }


def scenario_hash(scenario, ticks=0, repo_root="."):
    """Run twice; return (hash, deterministic_bool). Raises on CLI failure."""
    a = run_scenario(scenario, ticks, repo_root=repo_root)
    b = run_scenario(scenario, ticks, repo_root=repo_root)
    if a["returncode"] != 0 or b["returncode"] != 0:
        raise RuntimeError(f"scenario failed: {a['stderr'] or b['stderr']}")
    return a["state_hash"], a["state_hash"] == b["state_hash"]
