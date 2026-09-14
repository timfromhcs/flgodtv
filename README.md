<p align="center">
  <img src="docs/assets/banner.svg" alt="FLGODTV Technical Banner" width="100%">
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue.svg" alt="C++20">
  <img src="https://img.shields.io/badge/Vulkan-1.4-red.svg" alt="Vulkan 1.4">
  <img src="https://img.shields.io/badge/Godot-4.7.2%20Forward%2B-478cbf.svg" alt="Godot 4.7.2">
  <img src="https://img.shields.io/badge/CTest-49%2F49%20Passed%20(100%25)-brightgreen.svg" alt="CTest 49/49 Passed">
  <img src="https://img.shields.io/badge/Connectome-128.9M%20somas%2Fs-orange.svg" alt="128.9M somas/s">
  <img src="https://img.shields.io/badge/License-Apache%202.0%20%2F%20MIT-lightgrey.svg" alt="License">
</p>

---

## Project

**FLGODTV** is a deterministic, local-first, headless-capable simulation platform uniting biological connectome neural modeling, procedural world generation, multi-agent continuous evolution, and a Godot 4 Forward+ Vulkan presentation frontend.

The platform executes entirely independent of rendering. A C++20 simulation core maintains canonical state across physics, continuous world fields, genetics, multi-tier memory, sandboxed technology execution, and neural somatic leaky integration. Godot 4 serves strictly as a consumer of broadcast state over an adapter boundary.

---

## Status

As of commit `349a868` (verified on Windows 11 x64 with MSVC 19.44 / Ninja 1.13.2 and Godot 4.7.2):

- **Phase A (Backend Simulation & Infrastructure):** `100% VERIFIED` across all 15 gates defined in `GEMINI.md` Section 67.
- **Phase B (Frontend & Rendering):**
  - **Stage 16 (Godot Setup & Presentation Bridge):** `VERIFIED`
  - **Stage 17 (World Rendering):** `VERIFIED` (Terrain LOD/streaming, GPU MultiMesh vegetation, procedural water, weather GPU particles/fog, 4 lighting profiles).
  - **Stage 18 (Camera Director & Event Detection):** `NEXT` (In progress).
  - **Stage 19 (HUD & UI):** `PENDING`.
- **Test Suite Status:** **49 / 49 tests passed (100%)** via CTest and headless runner.

---

## Architecture

The system enforces strict architectural boundaries: the simulation core never depends on Godot or visual rendering.

```text
+-------------------------------------------------------------------------+
|                        FLGODTV SIMULATION CORE                          |
|  +-------------------+  +-------------------+  +---------------------+  |
|  | SimulationClock   |  | DeterministicRNG  |  | WorldState & Chunk  |  |
|  | (Fixed dt=1/60s)  |  | (SplitMix64)      |  | (fBm Elevation)     |  |
|  +-------------------+  +-------------------+  +---------------------+  |
|  +-------------------+  +-------------------+  +---------------------+  |
|  | PhysicsEngine     |  | Continuous Fields |  | WeatherSystem       |  |
|  | (L0/L1/L2 CCD)    |  | (Temp/Moist/Wind) |  | (Diurnal Cycle)     |  |
|  +-------------------+  +-------------------+  +---------------------+  |
|  +-------------------+  +-------------------+  +---------------------+  |
|  | EvolutionEngine   |  | QLearner & Memory |  | Programmable Tech   |  |
|  | (8-Trait Genome)  |  | (5-Tier Store)    |  | (Sandboxed 8-reg VM)|  |
|  +-------------------+  +-------------------+  +---------------------+  |
+------------------------------------+------------------------------------+
                                     |
                          ADAPTER BOUNDARY (Section 8)
                                     |
+------------------------------------+------------------------------------+
|                      EXTERNAL BRAIN DEPENDENCY                          |
|  +-------------------------------------------------------------------+  |
|  | MaleCNS Biological Connectome Interface (IFlyBrain)               |  |
|  | Ingests 125,507 somas from malecns/data-raw/2023-27-2 soma_sides  |  |
|  | Rate-coded leaky integration, optomotor wingbeat, PER feeding     |  |
|  +-------------------------------------------------------------------+  |
+------------------------------------+------------------------------------+
                                     |
                       JSON TELEMETRY / BRIDGE API
                                     |
+------------------------------------+------------------------------------+
|                    GODOT 4 FORWARD+ PRESENTATION                        |
|  +---------------------+ +--------------------+ +--------------------+  |
|  | 4-Camera Rig        | | Terrain Renderer   | | MultiMesh Renderer |  |
|  | (Global/Fly/Col/Evt)| | (L0/L1/L2 Stream)  | | (Flies & Flora)    |  |
|  +---------------------+ +--------------------+ +--------------------+  |
|  +---------------------+ +--------------------+ +--------------------+  |
|  | Procedural Water    | | Weather Particles  | | Lighting Profiles  |  |
|  | (Reflection/Ripple) | | (Fog / Rain GPU)   | | (LOW to CINEMATIC) |  |
|  +---------------------+ +--------------------+ +--------------------+  |
+-------------------------------------------------------------------------+
```

---

## Requirements

### Host Build Environment
- **Operating System:** Windows 10/11 x64 or Linux (Ubuntu 22.04+ / Debian 12+)
- **C++ Compiler:** C++20 compliant compiler:
  - MSVC 19.38+ (Visual Studio 2022) on Windows
  - GCC 12+ or Clang 16+ on Linux
- **Build System:** CMake 3.20+ and Ninja 1.11+
- **Vulkan:** Vulkan SDK 1.3+ / 1.4+ with `glslc` SPIR-V compiler
- **Python:** Python 3.10+ (for test validation and data preprocessing)

### Optional Presentation & Media Toolchain
- **Godot Engine:** Godot 4.7+ (Forward+ Vulkan renderer)
- **FFmpeg:** FFmpeg 6.0+ (for video encoding pipelines)
- **Blender:** Blender 4.0+ / 5.0+ (for procedural asset baking)

---

## Installation & Usage

### 1. End-User Standalone Installation (No Dev Tools Required)

End users do not need Visual Studio, CMake, Ninja, Python, Blender, or the Godot editor.

#### Windows
- **Installer:** Download and run `release/windows/FLGODTV-0.1.0-Setup.exe` (supports interactive install, desktop shortcut creation, uninstaller, or unattended setup via `--silent --install <path>`).
- **Portable ZIP:** Extract `release/windows/FLGODTV-0.1.0-windows-x64-portable.zip` and double-click `FLGODTV.bat` to launch the presentation, run headless simulation, or execute system self-tests.

#### Linux
- **Portable Archive:** Extract `release/linux/FLGODTV-0.1.0-linux-x64-portable.tar.gz` and execute `./run_flgodtv.sh`.

---

### 2. Developer Source Installation & Build

```bash
# Clone the repository with submodules
git clone --recursive https://github.com/timfromhcs/flgodtv.git
cd flgodtv

# Verify external connectome dataset baseline
git -C malecns rev-parse HEAD
# Expected baseline commit: daf8e2a9849cc77695b14bb6b9d4c02456cd3b3c
```

#### Windows (MSVC x64 Native Tools Command Prompt)
```cmd
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DFLGOD_ENABLE_VULKAN=ON
cmake --build build --config Release
```

#### Linux
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DFLGOD_ENABLE_VULKAN=ON
cmake --build build --config Release
```

#### Blender Procedural 3D Asset Pipeline
```bash
# Generate deterministic 3D morphological fly models, flora, and environment assets
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" -b --python scripts/blender_asset_generator.py -- --out-dir godot/assets/models --seed 42
```

#### Standalone Release Packaging
```bash
# Export Godot presentation pack, compile native installer, and run full install verification test
python scripts/package_release.py
```

---

## Headless Usage

The primary simulation binary runs without requiring an X11/Wayland display, desktop environment, or Godot editor:

```bash
# Display version and build configuration
./build/flgod --version

# Run comprehensive subsystem self-test (5 phases)
./build/flgod --self-test

# Validate physical and continuous invariants
./build/flgod --validate

# Run headless simulation for N ticks (e.g. 600 ticks)
./build/flgod --simulate 600

# Run multi-agent evolutionary harness for N generations
./build/flgod --evolve 5

# Run learning training harness for N episodes
./build/flgod --train 10

# Save deterministic state snapshot checkpoint
./build/flgod --checkpoint state_tick180.json 180

# Restore and resume simulation from checkpoint
./build/flgod --restore state_tick180.json 60

# Record deterministic replay log and verify bit-exact state reproduction
./build/flgod --replay replay.log 200
```

---

## Godot Usage

The visual frontend is decoupled from the simulation. To launch the interactive Forward+ Vulkan presentation:

```bash
# Launch interactive 4-camera presentation
godot --path godot/

# Execute headless frontend smoke test (verifies 4 cameras, MultiMesh, and world renderers)
godot --headless --path godot/ --script res://scripts/test_headless_frontend.gd
```

---

## Models

- **Fly-Brain Connectome:** Reconstructed *Drosophila melanogaster* male central nervous system dataset (`malecns`), loading somatic coordinates, hemisphere segregation, and morphological bounds from `malecns/data-raw/2023-27-2 soma_sides.csv`.
- **Local GGUF Models:** Compatible with GGUF v2/v3 instruct weights via llama.cpp backend adapter. Memory budget allocator and 5-stage sandboxed action validation pipeline enforce host isolation.

---

## Experiments & Benchmarks

All benchmark figures were recorded from executed binary runs on AMD Ryzen 7 7735HS (8C/16T) with AMD Radeon Graphics (RDNA2 0x1681):

| Subsystem | Metric | Verified Measurement | Realtime Factor |
| :--- | :--- | :--- | :--- |
| **MaleCNS Fly-Brain** | Leaky Soma Updates | **128.95M somas / sec** | ~129x at 100 Hz |
| **Multi-Agent Ecosystem** | 20 Agents, 4 Colonies | **9,418.45 ticks / sec** | ~157x at 60 Hz |
| **Simulation Core** | Clock & RNG Step | **28,078,845 ticks / sec** | ~467,981x at 60 Hz |
| **Programmable Tech VM** | Instruction Execution | **121.61 MIPS** | — |
| **Virtual Device Network** | Packet Routing | **5,418,204 packets / sec** | — |
| **Language System** | Utterance Processing | **16,706,762 utterances / sec** | — |
| **LLM Action Security** | 5-Stage Sandbox Gate | **170,387 validations / sec** | — |
| **Vulkan Host &rarr; Device** | Memory Transfer Bandwidth | **28,764 MB / sec** | — |

Detailed experiment manifests and raw JSON data are archived in [`evidence/windows/`](file:///C:/Users/hcsme/Desktop/Fly/evidence/windows/).

---

## Testing

CTest runs 52 automated test targets spanning unit, deterministic integration, physics, GPU compute, learning, evolution, camera directing, telemetry UI, and frontend validation:

```bash
# Execute the full test suite
ctest --test-dir build --output-on-failure
```

```text
100% tests passed out of 52

Total Test time (real) = ~2.4 sec
```

Key test suites:
- `DeterministicSimulationTest`: Bit-exact simulation hash across independent executions.
- `DeterministicPhysicsTest`: 500-step rigid body trajectory match and 100% midpoint crash recovery hash match.
- `DeterministicWorldTest`: Multi-octave procedural terrain and continuous field determinism.
- `DeterministicCameraTest`: Bit-exact deterministic camera pose replication across runs over 150 ticks.
- `EvolutionPopulationTest`: 25-generation drift, speciation isolation, and migration tracking.
- `VulkanComputeTest`: Bit-exact numerical equality (0.0 max absolute diff) between CPU reference and GPU compute kernel output.
- `TelemetryHUDTest`: Real simulation telemetry extraction with strict "N/A" fallback safety.
- `GodotFrontendSmokeTest`: Headless validation of 4-camera rig, MultiMesh instancing, LOD decimation, weather consequences, and automated UI/UX screenshot capture.

---

## Reproducibility

Every deterministic test run obeys the invariant:
$$\text{Seed} + \text{SimulationVersion} + \text{Inputs} \implies \text{Bit-Exact State Hash}$$

- PRNG streams are strictly partitioned: `world_seed`, `weather_seed`, `physics_seed`, `agent_seed`, `genome_seed`, `event_seed`, `learning_seed`.
- Midpoint crash recovery is verified: a restored snapshot from tick $T/2$ run to tick $T$ matches an uninterrupted continuous run to tick $T$ bit-for-bit.

---

## Known Limitations

1. **Hardware GPU Requirement for Vulkan Compute:** Real Vulkan compute kernels require a compatible physical Vulkan 1.3+ GPU (AMD, NVIDIA, or Intel). Headless cloud CI runners lacking a physical GPU report `VULKAN GPU TEST = NOT AVAILABLE` per `GEMINI.md` Section 103 rather than fabricating CPU mock results.
2. **Interactive Rendering:** Godot presentation requires a local display or virtual framebuffer (`xvfb` on Linux) for interactive windowed viewing. Headless test automation executes without display hardware.
3. **Automated Video Encoding Pipeline:** Stage 20 (automated headless FFmpeg video rendering pipeline) is the next engineering milestone. Stages 18 (Camera Director) and 19 (Telemetry HUD) are fully implemented and verified.

---

## Evidence

Every technical assertion in this document is traceable to executed code and persistent evidence logs:

- [docs/AUDIT.md](file:///C:/Users/hcsme/Desktop/Fly/docs/AUDIT.md) — Baseline repository audit and hardware configuration.
- [docs/STAGE_STATUS.md](file:///C:/Users/hcsme/Desktop/Fly/docs/STAGE_STATUS.md) — Detailed verification status for all engineering stages.
- [docs/BACKEND_GATE_REPORT.md](file:///C:/Users/hcsme/Desktop/Fly/docs/BACKEND_GATE_REPORT.md) — Phase A Section 67 Quality Gate completion report.
- [dependencies.lock.json](file:///C:/Users/hcsme/Desktop/Fly/dependencies.lock.json) — Pinned machine-readable dependency manifest.
- `evidence/windows/` — Raw execution outputs, benchmark JSON reports, and compiler version dumps.

---

## License

- Source code: Licensed under the [Apache License, Version 2.0](file:///C:/Users/hcsme/Desktop/Fly/LICENSES/APACHE-2.0.txt) or [MIT License](file:///C:/Users/hcsme/Desktop/Fly/LICENSES/MIT.txt).
- Connectome data: Ingested from external baseline `malecns` under its upstream research terms.
