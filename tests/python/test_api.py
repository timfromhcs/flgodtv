"""Python API smoke test (stdlib only): import, binary discovery, scenario run."""
import os
import sys

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))

from flgodtv import find_binary, run_cli, run_scenario, scenario_hash

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))


def main():
    binary = find_binary(ROOT)
    print("binary:", binary)
    r = run_cli("--version", repo_root=ROOT)
    assert r.returncode == 0, "version must exit 0"
    res = run_scenario("scenarios/profiles/minimal.json", repo_root=ROOT)
    assert res["returncode"] == 0, f"scenario failed: {res['stderr']}"
    assert res["telemetry"] is not None, "telemetry must be returned"
    assert res["telemetry"]["schema_version"] == 1, "telemetry schema version"
    h, det = scenario_hash("scenarios/profiles/minimal.json", repo_root=ROOT)
    assert det and h == res["state_hash"], "python-driven runs deterministic"
    print("hash:", h, "deterministic:", det)
    print("[PASS] python api smoke test")
    return 0


if __name__ == "__main__":
    sys.exit(main())
