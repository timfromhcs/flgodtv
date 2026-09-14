# MPE Telemetry (V1, schema version 1)

`EngineTelemetry::to_json()` emits: `schema_version`, `tick`, `alive`,
`spawned_total`, `died_total`, `moves`, `eats`, `communicates`, `mean_energy`,
`rules_fired` (per-rule cumulative counts).

## Export

- CLI: `flgod --scenario <path> [ticks] --telemetry-out <file>` writes
  `{scenario, telemetry, state_hash}`.
- Experiments: `scripts/run_experiment.py <scenario> [--ticks N] [--repeats R]
  [--id NAME]` runs repeats, asserts identical hashes, and writes
  `evidence/experiments/<id>.json` with telemetry, wall time, environment, and
  binary hash.
- Python: `flgodtv.run_scenario()` returns telemetry dicts.

## Current state

Schema covers engine-level population metrics. Per-module timing (brain timing,
module timing, memory) and species-level series are future work; the schema
version field guards that evolution. No fake series are emitted.
