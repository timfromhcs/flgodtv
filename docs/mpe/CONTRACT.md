# MPE Contract (V1)

Binary-compat is NOT promised in V1. Source-level and data-level contracts ARE:

## Concept contracts

- **MPEEngine**: owns `SimulationClock`, module registry, `SimulationScheduler`,
  `EventBus`, RNG streams. `initialize(Config)` / `shutdown()` explicit.
  `step()` executes the fixed system order exactly once per tick.
- **SimulationClock**: fixed `dt` (default 1/60 s), monotonic tick, no wall-clock.
- **DeterministicRNG**: SplitMix64/Xoshiro256++ streams partitioned per domain
  (world, weather, physics, agent, genome, event, learning, render). Same seed +
  same version + same input = same output.
- **Entity**: stable `EntityID` + `ArchetypeID` + component set. Never a
  species assumption.
- **Component**: plain serializable data (`to_json`/`from_json`), versioned,
  hashable, validated on load.
- **System**: one pipeline stage with explicit order (see scheduler list in
  mission Phase 10). Systems read components, never reach into each other.
- **Module/Environment**: `IEnvironmentModule` with deterministic
  `initialize/step/serialize/hash/enable` lifecycle.
- **Sensor**: `ISensor::sample(Environment, Entity) -> Observation`. MUST NOT
  mutate simulation state.
- **Observation**: plain data fed to brains.
- **Brain**: `IBrain` lifecycle `initialize/observe/step/intents/serialize/
  hash/reset`. MUST NOT mutate world state directly.
- **ActionIntent → ActionValidator → ActionSystem → Action**: intents are
  validated against capabilities + world state before execution.
- **Rule**: configurable, replaceable scenario logic (foraging, predation,
  reproduction, territory, ...). Fly-specific rules are scenario data, never
  engine assumptions.
- **Event**: deterministic ordering, serialization, replay support.
- **Scenario/Archetype**: JSON data (schema versioned). Engine never hardcodes
  archetypes.
- **Snapshot/Replay**: checkpoint holds ALL mutable state for exact continuation;
  `0→N` ≡ `0→N/2→restore→N` bit-exact.
- **Telemetry**: versioned schema, machine-readable export.
- **ComputeBackend**: `IComputeBackend`; CPU reference is trusted; accelerated
  output validated against it with recorded tolerances.
- **PresentationAdapter**: one-way mirror (state out, no control in beyond
  defined viewer commands like pause/checkpoint).

## Current-state mapping (honest)

IMPLEMENTED (pre-MPE, working, tested): SimulationClock, DeterministicRNG,
EntityID, EventBus, WorldState+checkpoint, World/procedural/physics/learning/
evolution/agents/language/technology subsystems, IFlyBrain+MaleCNSAdapter,
headless CLI, replay log, Vulkan compute + CPU reference, Godot presentation.

TO BUILD (Phases 4–27): generic Entity/Component registries, IEnvironmentModule
runtime, ISensor framework, IBrain generalization + adapters, ActionIntent
pipeline, SimulationScheduler, generic Rule engine, JSON archetypes/scenarios/
profiles, Snapshot/Replay generalization, experiment framework, versioned
telemetry schema, tests/edge harness, Python package.

Fly-specific types (`BrainSensoryInput` eyes/antennae, `BrainMotorOutput`
wings/proboscis) remain ONLY behind the MaleCNS adapter, never in core headers.
