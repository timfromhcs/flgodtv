# Initial Repository and Environment Audit

**Date:** 2026-09-13  
**Project:** FLGODTV  
**Host Operating System:** Microsoft Windows 11 Pro 64-Bit (10.0.26100)  
**Hardware Platform:** AMD Ryzen 7 7735HS with Radeon Graphics (16 logical processors), 20.25 GB RAM, AMD Radeon(TM) Graphics (DeviceID 0x1681, Driver 32.0.21043.12001)

---

## 1. Toolchain & Runtime Environment

| Component | Detected Version / Path | Status | Verification Source |
|---|---|---|---|
| **Operating System** | Microsoft Windows 11 Pro 64-Bit | VERIFIED | `Get-ComputerInfo` |
| **CPU** | AMD Ryzen 7 7735HS (8 Cores, 16 Threads) | VERIFIED | `Get-ComputerInfo` |
| **RAM** | 21,245,952,000 bytes (~20.25 GB usable) | VERIFIED | `Get-ComputerInfo` |
| **Disk Space** | C: ~153 GB free of 930 GB | VERIFIED | `Get-PSDrive C` |
| **GPU Hardware** | AMD Radeon(TM) Graphics (RDNA2, 0x1681) | VERIFIED | `Win32_VideoController` |
| **Vulkan API** | Instance: 1.4.357, Device: 1.4.315 (Driver: 2.0.353) | VERIFIED | `vulkaninfo --summary` |
| **Vulkan SDK** | 1.4.357.0 at `C:\VulkanSDK\1.4.357.0` | VERIFIED | `$env:VULKAN_SDK` |
| **C++ Compiler** | MSVC 19.44.35228 (Visual Studio 2022 Community 17.14.34) | VERIFIED | `vswhere.exe`, `cl.exe` |
| **Windows SDK** | 10.0.26100.0 / 10.0.22621.0 | VERIFIED | Registry & `Windows Kits\10\Include` |
| **CMake** | 4.4.0 (`C:\Program Files\Python314\Scripts\cmake.exe`) | VERIFIED | `cmake --version` |
| **Ninja** | 1.13.2 (`C:\Users\hcsme\AppData\Local\Microsoft\WinGet\Packages\Ninja-build.Ninja...`) | VERIFIED | `ninja --version` |
| **Git** | 2.54.0.windows.1 | VERIFIED | `git --version` |
| **Git LFS** | 3.7.1 | VERIFIED | `git lfs version` |
| **Python** | 3.14.6 (`C:\Program Files\Python314\python.exe`) | VERIFIED | `python --version` |
| **Pip** | 26.0 (`C:\Program Files\Python314\Scripts\pip.exe`) | VERIFIED | `pip --version` |
| **FFmpeg** | 9.0-full_build (`C:\Users\hcsme\AppData\Local\Microsoft\WinGet\Packages\Gyan.FFmpeg...`) | VERIFIED | `ffmpeg -version` |
| **Blender (Binary)** | 5.1.2 (`C:\Program Files\Blender Foundation\Blender 5.1\blender.exe`) | VERIFIED | `blender.exe --version` |
| **Blender (Source)** | 5.3.0 alpha (`blender/` at commit `349fc27092df84267860aacbb70987fe9adc1e47`) | VERIFIED | `git -C blender log -1` |
| **Godot Engine** | 4.7.2 Forward+ Vulkan (`C:\Users\hcsme\AppData\Local\Microsoft\WinGet\Packages\GodotEngine.GodotEngine_Microsoft.Winget.Source_8wekyb3d8bbwe\Godot_v4.7.2-stable_win64.exe`) | VERIFIED | `godot --version`, headless test |
| **R / Rscript** | Not installed in system PATH (data parsed directly via C++/Python) | N/A | `Get-Command R, Rscript` |

---

## 2. Baseline Fly-Brain Source Material Audit (`malecns`)

The repository initially contained `malecns/` at commit `daf8e2a9849cc77695b14bb6b9d4c02456cd3b3c` (upstream: `https://github.com/natverse/malecns.git`).

### Status: UNTOUCHED & PROTECTED
In compliance with Contract Section 8 ("Protect the Existing Fly-Brain"), the directory has been preserved without mutation, refactoring, or destructive modifications.

### Inventory of `malecns` Contents:
- **Nature of Material:** R package providing access to the Janelia Male Adult CNS Connectome dataset (`male-cns:v1.0`).
- **Data Files Included:**
  - `data-raw/2023-27-2 soma_sides.csv` (12.8 MB): Soma positions and hemisphere annotations for 168,000+ reconstructed neural bodies.
  - `data-raw/JRCFIB2022M.ply` (1.6 MB): Polygon mesh surface of the male fly brain/CNS volume.
  - `inst/landmarks/*.csv`: Microscopic spatial landmarks in micrometers and nanometers (e.g. `JRCFIB2022M_plotting_landmarks.csv`, `maleCNS_brain_FAFB_landmarks_um.csv`).
  - `data/*.surf.rda`: Pre-compiled R surface objects (`JRCFIB2022M.surf.rda`, `malecns.surf.rda`, `malecns_shell.surf.rda`, `malecnsvnc.surf.rda`).
- **License:** GPL-3 (recorded in `malecns/LICENSE.md`).
- **Buildability / Execution:** Requires R and `natverse`/`neuprintr` dependencies with Janelia NeuPrint credentials for live queries. Since R is not installed locally, local data files (`soma_sides.csv`, `JRCFIB2022M.ply`, landmarks) can be parsed directly in C++ / Python via our isolated adapter layer without altering `malecns`.

---

## 3. Repository State

- Initialized independent Git repository on `main` in `C:\Users\hcsme\Desktop\Fly`.
- Isolated from parent directory (`C:\Users\hcsme\Desktop`).
- Established standard FLGODTV directory layout defined in Contract Section 9.
- Added `.gitignore` preventing build/binary leakage.

---

## 4. Summary of Toolchain Verification & Integration
- **Stage 0 (Audit):** VERIFIED. Complete system, hardware, toolchain, and repository audit recorded.
- **Stage 1 (Toolchain):** VERIFIED. MSVC C++ compiler (19.44), CMake 4.4.0, Ninja 1.13.2, Vulkan SDK 1.4.357, Python 3.14.6, FFmpeg 9.0, Blender 5.1.2 binary, Blender 5.3.0 alpha source tree (`blender/`), and Godot 4.7.2 are verified with evidence files in `evidence/toolchain/`.
- **Blender 3D Pipeline:** VERIFIED. Headless procedural asset generation (`scripts/blender_asset_generator.py`) generates deterministic 3D morphological fly models, flora shrubs, and rocks with 100% bit-exact SHA-256 replication across independent runs.
- **Standalone Packaging:** VERIFIED. Standalone Windows installer (`FLGODTV-0.1.0-Setup.exe`), Windows portable ZIP, and Linux portable tarball generated and verified via clean-install execution test (`evidence/windows/install_test_report.json`).
