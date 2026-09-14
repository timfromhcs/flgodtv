# Final Verification — Release Candidate 0.1.0

- **Execution commit:** `53384aabb444885100bde45e207b5ef5b8669af5` (2026-09-14)
- **Prior HEAD CI:** `8fbce67` — GitHub run 34835383156, success, Windows + Linux
- **Tree:** clean at commit; no secrets committed
- **Source tree state:** `src/`, `include/`, `tests/`, `godot/`, `scripts/`, `shaders/`,
  `tools/installer/`, `malecns/` baseline pinned at `daf8e2a9849cc77695b14bb6b9d4c02456cd3b3c`

## Toolchains (verified)

MSVC 19.44.35228 (VS 2022 17.14.34), CMake 4.4.0, Ninja 1.13.2,
Vulkan SDK 1.4.357.0 (device 1.4.315), Python 3.14.6, FFmpeg 9.0,
Blender 5.1, Godot 4.7.2 Forward+. OS: Windows 11 x64.
CPU: AMD Ryzen 7 7735HS. GPU: AMD Radeon(TM) Graphics RDNA2 (0x1681).
Evidence: `evidence/toolchain/`.

## Dependencies & licenses

`dependencies.lock.json` + `docs/DEPENDENCIES.md`: MSVC/CMake/Ninja/Vulkan/Python/
FFmpeg/Blender (verified), malecns baseline (GPL-3.0, protected), nlohmann/json
3.11.3 (MIT), JoltPhysics 5.6.0 (MIT). Code: Apache-2.0/MIT dual.

## Models

No real LLM runtime or model weights are bundled. The LLM layer provides GGUF
parsing, model management, and a 5-stage sandboxed action gate (verified by
6 LLM test targets using synthetic fixtures). Documented as NOT INTEGRATED for
real model inference — a parser is not an inference engine.

## Results

- **Windows (Ninja Release, clean rebuild):** 55/55 CTest PASS (~13.1 s).
  Compiler diagnostics: only C4189/C4100 unused-variable warnings in test code;
  zero errors. (Prior "warning-free" claims are corrected: test-code warnings exist.)
- **Linux:** local build UNAVAILABLE (no Linux env); prior HEAD CI Linux job
  succeeded; new commit pending CI re-run.
- **CPU/headless:** PASS (`--self-test` 5/5, `--validate`, `--simulate 120`,
  checkpoint/restore hash `0xbb4b22bf4769a206`, replay bit-exact).
- **Determinism:** PASS — simulation `2326199170333143766`,
  physics 500-step `0x7fe83107ca6891d4`, evolution 25-gen `0xbaf2e641efe02fec`,
  all with midpoint crash-recovery match.
- **Vulkan GPU (physical):** PASS — init, 1 MB buffer roundtrip, vector-add and
  field-diffusion vs CPU reference with 0.0 max abs difference.
- **Godot:** PASS (windowed, real GPU) — smoke + Visual V2, fresh screenshots,
  `visual_validation.json` with hashes/dimensions/variance.
- **Integration:** PASS — protocol roundtrip, bridge ingestion, 8 integration tests.
- **Video:** PASS — 90/90 frames 1280x720 @30 FPS, H.264, 3.0 s, 7,410 bytes,
  SHA-256 `2dc163240521034a6b9bc6414f7cad2c67e7a4c441e8511b94017dbf20413868`
  (deterministic re-run reproduced the identical hash).
- **Release:** rebuilt from execution commit; installer + portable ZIP + Linux
  tarball; installed-binary `--version/--self-test/--validate/--simulate 60`
  PASS; installed frontend PCK main-scene run PASS; SHA256SUMS verified;
  1-byte tamper correctly detected (FAIL on modified copy, PASS on original).
- **CI:** workflow `.github/workflows/ci.yml` = CPU/headless CI (excludes 3 Vulkan
  + Godot smoke tests; Godot absent in cloud). HEAD run 34839952661 green on
  Windows + Linux. Matrix in `evidence/ci/coverage_matrix.json`.
- **Documentation:** synchronized (README 55/55, checksums, display/GPU notes;
  inventory generated; this file). Historical stage counts remain in
  `STAGE_STATUS.md` labeled historical.

## Known limitations

1. Linux verified via CI only (no local Linux environment).
2. Cloud CI has no GPU/Godot/display: those targets are NOT_AVAILABLE there.
3. No real LLM inference runtime or model weights bundled.
4. Godot captures need display + Vulkan GPU; resolution follows window size.
5. Test-code C4189/C4100 warnings (benign, tracked).
6. GitHub Release `v0.1.0` published with all three packages
  (`https://github.com/timfromhcs/flgodtv/releases/tag/v0.1.0`).
