# MPE Components (V1)

Components are plain versioned data attached to entities. Minimum set:

Transform, Physics, Biology, Genome, Needs, Metabolism, Sensors, Brain,
Memory, Learning, Social, Communication, Reproduction, Inventory, Technology,
Culture, Lifecycle, Custom.

## Registries

- `ComponentRegistry`: type → factory/serializer/validator/version.
- `ArchetypeRegistry`: archetype JSON → component composition.
- `EntityRegistry`: stable IDs, deterministic iteration order (sorted by
  `EntityID`), spawn/despawn with event emission.

## Requirements

Deterministic iteration, stable IDs, JSON serialization/deserialization,
FNV-style state hashing, load-time validation, schema versioning.

## Current state

No generic registries exist yet. Existing equivalents: `EntityID` +
`EntityIDAllocator` (`core/entity_id.hpp`), `Agent` state bags (`agents/`),
`Genome` serialization (`evolution/`). Migration maps these onto components
without breaking current tests.
