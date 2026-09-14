# MPE Determinism (V1)

Formal invariant:

```text
SimulationVersion + ScenarioVersion + MasterSeed + ModuleConfiguration + Inputs
= DeterministicResult (bit-exact state hash)
```

## Requirements

- Partitioned RNG streams per domain; no unseeded randomness, no wall-clock,
  no thread-dependent ordering in state-affecting paths.
- Deterministic entity/component iteration (sorted stable IDs).
- Deterministic event ordering + serialization.
- Stable serialization before hashing (canonical JSON key order where hashed).
- Replay: recorded inputs reproduce the exact state stream.
- Checkpoint/restore: `0→N` ≡ `0→N/2→save→restore→N/2→N` bit-exact, including
  telemetry and event streams where applicable.

## Current state (verified baseline)

SplitMix64+Xoshiro256++ streams, deterministic sim/world/physics/camera/
population tests, midpoint crash-recovery tests, replay log — all passing
(55/55, `evidence/v1/baseline/`). These tests are the determinism lock for
every later phase: any refactor that changes a hash fails the gate.

## Non-goals

Rendered pixels are NOT required bit-exact (capture timing varies); only
simulation state, telemetry, and event streams are covered by the invariant.
