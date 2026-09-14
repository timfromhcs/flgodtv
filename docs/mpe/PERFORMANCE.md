# MPE Performance (V1)

All numbers must come from executed runs on recorded hardware. Baseline
(`evidence/v1/baseline/benchmarks.json`, 2026-09-14, Ryzen 7 7735HS):

- Core step: ~14,239 ticks/s full-physics benchmark binary
  (prior verified claim 28,078,845 ticks/s = clock/PRNG micro-step only)
- See `bench_*.log` for brain/ecosystem/tech/language/LLM/learning figures.

## Scale testing (Phase 17)

Measure 10 / 100 / 1,000 / 10,000 / 100,000 entities + local practical maximum.
Only evidence-backed optimizations: cache-friendly structures, spatial indexing,
broad phase, batching, deterministic parallelism where safe, LOD, population
simulation.

## Simulation LOD

LOD0 population → LOD1 simplified agent → LOD2 full agent → LOD3 detailed
physics → LOD4 detailed brain. LOD transitions preserve state per an explicit
contract (existing L0/L1/L2 physics fidelity is the starting point).

## Rules

- Never sacrifice determinism for speed without a defined, tested contract.
- Never extrapolate (realtime factors only for the measured workload).
- Benchmarks record hardware/OS/compiler/commit/seed/config/workload.
