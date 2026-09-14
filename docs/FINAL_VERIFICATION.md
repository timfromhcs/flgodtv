# Final Verification — V1 (MPE Universal Platform)

- **Version:** `1.0.0` (CMake, app `v1.0.0-v1`, Python `1.0.0`, installer, scenarios)
- **Toolchains:** MSVC 19.44.35228, CMake 4.4.0, Ninja 1.13.2, Vulkan SDK 1.4.357.0,
  Python 3.14.6, FFmpeg 9.0, Blender 5.1, Godot 4.7.2 Forward+ (Windows 11 x64,
  Ryzen 7 7735HS, AMD Radeon RDNA2)

## Results (all executed, see evidence)

- **Clean-room build:** 128 targets from scratch, zero errors → **61/61 CTest PASS**.
- **Determinism:** pre-V1 suites + MPE repeat runs + checkpoint bit-exactness
  (entities, brains incl. MaleCNS adapter, rule state, counters, genome traits).
- **Memory safety:** ASan-verified fix of a heap-use-after-free in the rule death
  path and a container-overflow in connectome restore sizing (both caught by new
  tests; Linux glibc abort resolved).
- **Scenarios:** 8 + 3 profiles run data-driven on one core
  (`MPEEngineTest` executes all; `evolution_lab`: 35 births/17 deaths).
- **Scale:** 500 entities × 200 ticks in ~0.4 s, deterministic
  (`evidence/experiments/scale_baseline_500.json`).
- **Edge:** 13/13 black-box checks (`EdgeHarnessTest`).
- **Python:** installs via pip, imports clean, drives scenarios (`PythonApiTest`,
  `evidence/v1/python_install.json`).
- **Godot:** windowed smoke + Visual V2 with validated fresh captures; viewer
  ingests MPE `--live-export` snapshots (bridge-connected run verified).
- **Video:** 90/90 frames 720p30 H.264, deterministic SHA-256 (re-verified).
- **Release:** `FLGODTV-1.0.0-Setup.exe` + portable ZIP + Linux tarball rebuilt
  from this commit; installed-binary checks PASS; SHA256SUMS verified; 1-byte
  tamper detected.
- **CI:** CPU/headless matrix (Vulkan/Godot excluded in cloud, documented).

## Known non-critical limitations

Linux via CI only; cloud CI has no GPU/Godot/display; no real LLM inference
runtime bundled (parser/manager/gate only); Godot captures need display+GPU;
test-code C4189/C4100-class warnings only; rendered pixels (not state) excluded
from bit-exactness by contract.
