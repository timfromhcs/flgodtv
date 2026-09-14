# MPE Extensions (V1)

Extension points (all replaceable without core changes):

- `IEnvironmentModule` (custom fields, e.g. pollution, radiation)
- `ISensor` / custom observations
- `IBrain` providers
- `IAction` types + validators
- Rules (scenario JSON + rule plugins)
- `GenomeSchema` traits, mutation/crossover/selection/speciation strategies
- `IComputeBackend` implementations
- Presentation adapters (Godot viewer is one adapter)

## Rules for extensions

- Registered by name + version; scenario references resolve or fail loudly.
- Extensions participate in hashing/checkpointing like built-ins.
- No extension may introduce hidden global state or unseeded randomness.
- Test-only fakes are labeled `mock_` and never presented as real backends.
