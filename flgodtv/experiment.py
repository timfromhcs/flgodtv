"""Entry point for the flgodtv-experiment console script (delegates to scripts/)."""
import runpy
import sys
from pathlib import Path


def main():
    script = Path(__file__).resolve().parent.parent / "scripts" / "run_experiment.py"
    sys.argv[0] = str(script)
    runpy.run_path(str(script), run_name="__main__")


if __name__ == "__main__":
    main()
