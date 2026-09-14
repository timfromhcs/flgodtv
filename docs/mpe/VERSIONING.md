# MPE Versioning (V1)

Versioned together but recorded separately:

- `MPE_API_VERSION` (core source contract)
- `SimulationVersion` (state schema; existing `core/version.hpp`)
- `ScenarioVersion` (per scenario document)
- Module versions (per module, in checkpoints)
- Brain provider versions (in checkpoints/experiments)
- `ScenarioSchemaVersion` (JSON schema)
- Telemetry schema version
- Release version (single V1 number across CMake/app/Python/installer/scenario
  schema/MPE API — Phase 32)

## Rules

- State schema bumps MUST keep a migration path or a clear break note; old
  checkpoints declare their versions and load only if compatible.
- Checkpoints record every version above; experiments record them plus backend,
  hardware, and software revision.
- No stale `0.x` references once V1 is declared (Phase 32 audit).
