# CURRENT STATE AUDIT — 2026-09-14

Commit: `8fbce67219dcd9f1daea1efeb99d1eaf52266da7` (branch `main`, tree CLEAN)
Remote: `https://github.com/timfromhcs/flgodtv.git`
Submodule `malecns`: NOT INITIALIZED locally (`-daf8e2a...`); pinned commit `daf8e2a9849cc77695b14bb6b9d4c02456cd3b3c` per `.gitmodules`/`dependencies.lock.json`. Brain loader tests use `malecns/data-raw/2023-27-2 soma_sides.csv` — file presence UNVERIFIED in this checkout (must confirm; tests will reveal).

## Test graph truth (source of truth: CMakeLists.txt + ctest -N)
- `CMakeLists.txt` registers 55 `add_test` names: 53 unconditional C++ + 2 conditional Godot (`GodotFrontendSmokeTest`, `GodotVisualV2VerificationTest`, gated on `GODOT_EXECUTABLE`).
- Existing `build/ninja-release`: `ctest -N` reports **Total Tests: 55** → Godot executable WAS found at configure time. **Current truth = 55.**
- `evidence/testing/test_inventory.json` GENERATED from CMakeLists (machine-readable, not hardcoded).
- Classification: CORE 4, DETERMINISM 4, WORLD 4, PHYSICS 4, HEADLESS 1, VULKAN 3, LEARNING 5, EVOLUTION 4, AGENTS 4, LLM 6, LANGUAGE 3, TECHNOLOGY 4, BRAIN 3, CAMERA 1, UI 1, VIDEO 1, INTEGRATION 1, GODOT 2. Integration-exec tests = 8 (HeadlessModes, LearningParallel, MultiAgentSimulation, LanguageTransmission, TechnologyIntegration, BrainAgentIntegration, MultiAgentEcosystem, LLMIntegration). Deterministic tests = 5 (4 `Deterministic*` + `EvolutionPopulationTest` in `tests/deterministic/`).
- CI (`ci.yml`) runs `ctest -E "(VulkanInitTest|VulkanBufferTest|VulkanComputeTest|GodotFrontendSmokeTest)"` → ~49-50 tests in CI, NOT the full 55. CI must be labeled "CPU/headless CI", not "full suite". Note: `GodotVisualV2VerificationTest` is NOT excluded in CI but requires Godot binary (absent in cloud) → CI configure would produce 53 tests; needs verification against actual CI logs.

## Documentation contradictions (CURRENT, must fix AFTER verification)
- `README.md`: badge `CTest-53/53`, text "53 test targets", but Repository Layout says "55 unit..." → STALE. Truth = 55 local / 53 without Godot / ~49-50 in CI.
- `STAGE_STATUS.md`: stages 16-19 claim `52/52`; stages 20-21 claim `55/55`; stage 22 claims `53/53` local. HISTORICAL counts unlabeled → must label historical per contract §33.
- `docs/VISUAL_V2_AUDIT.md`: pre-implementation BASELINE (states PARTIAL/MISSING) vs STAGE 29 claiming V2 VERIFIED → preserve audit as BASELINE, current state belongs in `VISUAL_V2_FINAL_VERIFICATION.md` (MISSING).
- `docs/FINAL_VERIFICATION.md`: MISSING (contract §35).
- `release_manifest.json`: MISSING at repo root (contract §48).
- `evidence/visual_v2/visual_validation.json`: MISSING (contract §18) — only `manifest.json` with hardcoded `"PASS"` strings, no sha256/dimensions/content metrics.
- `evidence/testing/current_test_inventory.json`: now created as `evidence/testing/test_inventory.json` (this audit); docs must reference it, not hardcoded counts.

## Build / toolchain (to be re-verified by clean build)
- `build/ninja-release` EXISTS (stale). Clean rebuild required.
- Toolchain evidence present under `evidence/toolchain/` (compiler, cmake, ninja, python, vulkaninfo, godot-version, ffmpeg, blender + blender deterministic pipeline JSON). Freshness vs current commit UNVERIFIED until rebuild.
- Local GPU: AMD Radeon RDNA2 present per prior evidence; `vulkaninfo` instance 1.4.357 confirmed this session. Real Vulkan execution possible locally; cloud CI = NOT_AVAILABLE (correctly handled in workflow).

## Frontend / visual / video (to be re-verified by execution)
- Godot 4.7.2 installed locally (WinGet package dir exists). Scripts use `assert()` (weak release-mode semantics) → contract §16 HARDENING REQUIRED (`test_headless_frontend.gd`, `test_visual_v2.gd`).
- `evidence/visual_v2/manifest.json`: hardcoded PASS values, no image hashes/dimensions → must regenerate from execution (§17-19).
- `evidence/video/render_manifest.json`: 90 frames, sha `2dc16324...`, size 7410 bytes — freshness vs current commit UNVERIFIED; frame hashes MISSING (§23).
- Backend→frontend bridge + E2E path: implementation exists; live-data consumption UNVERIFIED this session.

## Release (to be re-verified)
- Packages EXIST: `release/windows/FLGODTV-0.1.0-Setup.exe` (87.9 MB), `windows/...portable.zip` (87.9 MB), `linux/...tar.gz` (163,855 bytes), `release/checksums/SHA256SUMS.txt` (3 lines). Freshness vs current HEAD UNVERIFIED; checksums in README match the checksum file (to be re-hashed after rebuild). No GitHub Release check performed yet (needs `gh`).

## Security
- User message contains a `ghp_` token. NEVER commit tokens. `git status` clean; no secrets found in repo files inspected so far. Secret scan + `gh` CI check pending. Token MUST be revoked/rotated by owner if real.

## Status labels
- CURRENT: commit 8fbce67, tree clean, 55 configured tests, packages present, evidence present but freshness UNVERIFIED.
- VERIFIED (this session): test-graph count 55 via `ctest -N`; vulkan instance 1.4.357; cmake 4.4.0; ninja 1.13.2; python 3.14.6; ffmpeg 9.0; Godot package dir present.
- IMPLEMENTED_BUT_UNVERIFIED: clean build, full ctest, determinism reruns, Godot runs, visual validation, video regen, release rehash, CI head check.
- HISTORICAL: all STAGE_STATUS counts, benchmark figures, screenshots, manifests (until rerun).
- BLOCKED: none yet. Linux build/CI-head check require network/gh (attempt next).
