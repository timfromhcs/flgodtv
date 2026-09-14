# MPE Brains (V1)

The core depends ONLY on the generic brain contract:

```cpp
class IBrain {
  initialize(config); observe(Observation);
  step(dt); intents() -> [ActionIntent];
  to_json/from_json; compute_hash; reset;
};
```

## Providers (adapters, replaceable per archetype)

- `malecns`: existing MaleCNS connectome adapter (fly I/O mapped at the
  boundary; fly types never leak into core).
- `scripted`: deterministic scripted policy (for tests and simple fauna).
- `rl_policy`: learned policy (Q-learner from `learning/`).
- `llm`: event-driven local-LLM provider behind the sandboxed action gate.
- `hybrid` / `external`: composition / out-of-process (contract-defined later).

## Rules

- Brains MUST NOT mutate world state; they emit `ActionIntent` only.
- Brain version + provider recorded in checkpoints/experiments.
- God Fly "teacher" capability becomes a scenario-level teaching module
  (Phase 19/20), not a core special case.

## Current state

`IFlyBrain` exists but is fly-specific (eyes/wings/proboscis types). The generic
`IBrain` + `Observation`/`ActionIntent` types are TO BUILD (Phase 7); the
MaleCNS adapter is then ported to implement it.
