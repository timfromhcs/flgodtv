# Dependencies Policy & Record

This document records all external dependencies used in FLGODTV, in accordance with Section 10 of `GEMINI.md`.

## System & Toolchain Dependencies (Detected & Verified)

| Name | Source URL | Version | License | Platform Support | Build / Install Method | Purpose | Verification Command |
|---|---|---|---|---|---|---|---|
| **MSVC Compiler** | https://visualstudio.microsoft.com/ | 19.44.35228 | Proprietary MS | Windows x64 | Visual Studio Installer | C++20 Core Compilation | `cl.exe` |
| **CMake** | https://cmake.org/ | 4.4.0 | BSD-3-Clause | Windows, Linux, macOS | Python pip / Binary | Build configuration | `cmake --version` |
| **Ninja** | https://ninja-build.org/ | 1.13.2 | Apache-2.0 | Windows, Linux, macOS | Winget / Binary | Fast native build tool | `ninja --version` |
| **Vulkan SDK** | https://vulkan.lunarg.com/ | 1.4.357.0 | Apache-2.0 / MIT | Windows, Linux | LunarG Installer | Compute shader compilation & Vulkan API | `vulkaninfo --summary` |
| **Git** | https://git-scm.com/ | 2.54.0 | GPL-2.0 | Windows, Linux, macOS | Git for Windows | Version control | `git --version` |
| **Git LFS** | https://git-lfs.com/ | 3.7.1 | MIT | Windows, Linux, macOS | Package / Installer | Large file versioning | `git lfs version` |
| **Python** | https://www.python.org/ | 3.14.6 | PSF | Windows, Linux, macOS | Python Installer | Scripting, data pipeline | `python --version` |
| **FFmpeg** | https://ffmpeg.org/ | 9.0 | LGPL/GPL | Windows, Linux, macOS | Gyan.dev Winget build | Video rendering & encoding | `ffmpeg -version` |
| **Blender** | https://www.blender.org/ | 5.1 | GPL-3.0 | Windows, Linux, macOS | Blender Foundation | 3D asset pipeline | `blender --version` |

## Baseline Connectome Dataset (External Baseline)

| Name | Source URL | Version/Commit | License | Platform Support | Build Method | Purpose | Verification Command |
|---|---|---|---|---|---|---|---|
| **malecns** | https://github.com/natverse/malecns.git | daf8e2a9849cc77695b14bb6b9d4c02456cd3b3c | GPL-3.0 | Cross-platform | Preserved external baseline | Connectome reference data (Janelia male CNS) | Git commit check |

## Third-Party Libraries (Vendored / Pinned)

| Name | Source URL | Version/Commit | SHA256 Hash | License | Platform Support | Build Method | Purpose | Verification Command |
|---|---|---|---|---|---|---|---|---|
| **nlohmann/json** | https://github.com/nlohmann/json | v3.11.3 | `9bea4c8066ef4a1c206b2be5a36302f8926f7fdc6087af5d20b417d0cf103ea6` | MIT | Cross-platform | Header-only | Serialization of state, checkpoints, manifests | C++ include check |

