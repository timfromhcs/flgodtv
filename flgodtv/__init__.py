"""FLGODTV Python interface (V1).

Honest scope: this package drives the compiled headless CLI as a subprocess
(scenario execution, telemetry collection) and re-exports the experiment
runner. It does NOT reimplement simulation, brains, or physics in Python.
Requires a built ``flgod`` binary (see ``find_binary``).
"""
from .api import find_binary, run_cli, run_scenario, scenario_hash, __version__

__all__ = ["find_binary", "run_cli", "run_scenario", "scenario_hash", "__version__"]
