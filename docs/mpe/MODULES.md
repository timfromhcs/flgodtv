# MPE Modules (V1)

A Module is a registered, enable/disable-capable unit with deterministic
lifecycle (`initialize`, `step(dt, tick)`, `to_json`/`from_json`,
`compute_hash`, `name`, `version`). Modules are ordered by the
`SimulationScheduler`; order is explicit and inspectable.

## Environment modules (from existing World)

TerrainModule, ClimateModule, WeatherModule, WindModule, TemperatureModule,
HumidityModule, MoistureModule, WaterModule, FireModule, EcologyModule,
ResourceModule, CustomFieldModule.

Current source: `include/flgod/world/` (fields, chunks, procedural generator,
weather system). Migration wraps each field system as an `IEnvironmentModule`
without changing generated values (existing world tests are the lock).

## Agent modules

PerceptionSystem, BrainSystem, DecisionSystem, MetabolismSystem, SocialSystem,
LearningSystem, ReproductionSystem, TechnologySystem.

Current source: `agents/`, `brain/`, `learning/`, `language/`, `technology/`.

## World-service modules

PhysicsSystem (wraps `physics/` L0/L1/L2), EvolutionSystem (wraps `evolution/`
with `GenomeSchema`/strategy registries), RuleSystem (Phase 11),
TelemetrySystem (Phase 26), CheckpointSystem (Phase 16).

## Rules for modules

- Replaceable: a scenario lists modules; unknown/missing module = load error,
  never silent skip.
- Composable: modules declare dependencies; scheduler validates the graph.
- Hashable: module state contributes to the global state hash.
- Versioned: module version recorded in checkpoints and experiments.
