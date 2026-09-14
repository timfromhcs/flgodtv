# MPE Architecture (V1 target)

The Multi-Agent Simulation Platform (MPE) is a deterministic, modular,
scenario-driven simulation core. This document describes the TARGET architecture.
Current implementation state per file/phase is tracked in `docs/mpe/CONTRACT.md`
and `evidence/v1/`.

## Layering (hard boundaries)

```text
scenarios/            data-driven scenario + archetype JSON (no C++)
     |
profiles/             reusable module bundles (compose, don't duplicate)
     |
MPE core (include/flgod/mpe/)   species-agnostic: entities, components,
     |                          systems, scheduler, events, rules, brains,
     |                          sensors, actions, persistence, telemetry
     +-- adapters              MaleCNS fly brain, scripted brains, RL policies
     |
presentation (godot/) mirrors state via protocol; never owns it
```

Rules:

- The MPE core MUST NOT assume an entity is a fly, human, robot, NPC, or insect.
- Fly-specific behavior lives in `scenarios/01_drosophila_ecosystem/` + adapters.
- The core MUST NOT depend on Godot, rendering, or any model file.
- All randomness originates from explicit seeded streams (`DeterministicRNG`).
- No hidden global state; explicit init/shutdown; explicit deterministic order.

## Core concepts (defined in CONTRACT.md)

MPEEngine, SimulationClock, DeterministicRNG, Entity, Component, System, Module,
Environment, Sensor, Observation, Brain, ActionIntent, Action, Rule, Event,
Scenario, Archetype, Snapshot, Replay, Telemetry, ComputeBackend,
PresentationAdapter.

## Migration strategy (adapters first, no breakage)

Existing subsystems (`world/`, `physics/`, `agents/`, `brain/`, `learning/`,
`evolution/`, `language/`, `technology/`) keep working behind new MPE interfaces:

- `MaleCNSAdapter` implements generic `IBrain` (fly I/O mapped to/from
  generic `Observation` / `ActionIntent`).
- Existing `World` becomes the default `EnvironmentRuntime` module set.
- Existing `Agent` state maps to MPE `Component` bags; `AgentManager` logic
  migrates into ordered `System` stages.
- No existing test may break during migration; adapters are removed only after
  equivalent MPE-native functionality is verified.
