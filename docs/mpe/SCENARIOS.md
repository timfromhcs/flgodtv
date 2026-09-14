# MPE Scenarios (V1)

Scenarios are JSON data validated against `scenarios/schema/scenario.schema.json`.
Defining a scenario MUST NOT require modifying C++ source.

## Scenario document

- `scenario_version`, `simulation_version`, `master_seed` (+ per-domain seeds)
- `environment`: module list with parameters + enable flags
- `populations`: archetype references + counts + spawn rules
- `brains`: provider per archetype (`malecns`, `scripted`, `rl_policy`, `llm`, ...)
- `sensors`, `actions`, `rules`, `evolution`, `learning`, `social`, `technology`
- `presentation`: viewer config (layers, camera defaults)

## Planned scenarios (Phase 13)

01_drosophila_ecosystem, 02_predator_prey, 03_ant_colony, 04_artificial_life,
05_robot_society — all RUNNING on the same core without source changes
(`MPEEngineTest` executes every `scenarios/*.json` + `scenarios/profiles/*`).
06_evolution_lab, 07_neural_lab, 08_custom_research are future work requiring
the evolution/specialist generalization phases.

Reusable profiles in `scenarios/profiles/`: minimal, social, benchmark
(500 entities × 200 ticks in ~0.4 s, deterministic).

## Current state

No `scenarios/` directory exists yet (see `evidence/v1/repository_inventory.json`
gaps). The default simulation (current `SimulationConfig` + `flgod --simulate`)
is the migration baseline and becomes `01_drosophila_ecosystem` data.
