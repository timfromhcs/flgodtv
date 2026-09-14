# FLGODTV — Visual V2 Comprehensive Audit & Engineering Plan

**Document Version:** 2.0.0  
**Audit Timestamp:** 2026-09-14T11:43:00+02:00  
**Audited Commit:** `e88192acc7b7f9491bf43aefb4c4f764b0830d84`  
**Target Goal:** Visual V2 Presentation, Rendering, Map, Telemetry, and Cinematic Upgrade (`DETERMINISTIC_END = TRUE`)

---

## 1. Executive Summary

This audit establishes the factual baseline of FLGODTV for the Visual V2 upgrade. In accordance with `GEMINI.md` and the Visual V2 contract, every subsystem is categorized into one of six strict states:
1. `VERIFIED`: Proven by executed automated tests and tangible artifacts.
2. `IMPLEMENTED BUT UNVERIFIED`: Code exists in repository but lacks dedicated visual/functional verification evidence.
3. `PARTIAL`: Code exists but covers only a subset of Visual V2 requirements.
4. `BROKEN`: Code fails compilation, crashes at runtime, or violates deterministic/visual constraints.
5. `MISSING`: Required subsystem does not yet exist.
6. `UNKNOWN`: Unchecked external or uninspected dependency.

---

## 2. Subsystem Audit Matrix

| Subsystem | State | Current Implementation Status | Visual V2 Gap / Required Actions |
|---|---|---|---|
| **C++ Core Engine & Determinism** | `VERIFIED` | 52/52 CTest passing. Monotonic clock, deterministic SplitMix64+Xoshiro256++ PRNG, bit-exact replay & crash recovery. | None. Must be preserved unchanged as canonical authority. |
| **C++ Camera Director & Channels** | `VERIFIED` | 4 presentation channels, 11 shot types, priority sorting, terrain avoidance, hysteresis hold. Passed `CameraDirectorTest` & `DeterministicCameraTest`. | Needs shot scoring improvements and dynamic subject prediction for autonomous documentary broadcast. |
| **C++ Event System & Detection** | `PARTIAL` | `EventDetector` with 11 event types (`SimulationEventType`). Deterministic priority sorting and history ring-buffer. | Visual V2 specifies additional event categories: `Combat`, `Predation`, `Discovery`, `ResourceDiscovery`, `Construction`, `Migration`, `Fire`, `Flooding`, `UnusualBehavior`. |
| **C++ State Exporter (`--export-state`)** | `VERIFIED` | Exports clock, weather, colonies, agents, God Fly, camera channels, and telemetry snapshots. | Needs export of canonical continuous terrain elevation grid and world field layers (temperature, moisture, ecology) for map synchronization. |
| **Godot 4 Forward+ Vulkan Toolchain** | `VERIFIED` | Godot 4.7.2 Forward+ Vulkan renderer verified on AMD Radeon RDNA2 (Vulkan 1.4.315). Minimal render proof captured. | None. Base engine is rock-solid. |
| **Terrain Mesh & Biomes (Terrain V2)** | `PARTIAL` | `chunk_size=48`, `cell_size=2.5`, Whittaker elevation color bands, LOD0/LOD1/LOD2 generation, backend grid ingestion. | Needs material blending (rock, soil, grass, sand, snow), slope-aware shading, biome classification, normal generation, and distance detail. |
| **Water Presentation (Water V2)** | `PARTIAL` | Transparent refracting water plane (`surface_size=350.0`), subtle vertical ripple animation. | Needs depth visualization (shoreline transparency transition), foam approximation at water edge, flow cue, and wetness integration with terrain. |
| **Vegetation & Ecology (Vegetation V2)** | `PARTIAL` | MultiMesh instancing of `flora_shrub.glb` across 65m radius (118 instances placed, underwater rejected). | Needs rich variety: grass, flowers, shrubs, dead vegetation, and rocks scattered based on environmental field sampling (moisture, elevation, biome). |
| **Agent Visuals & Behaviors (Agent V2)** | `PARTIAL` | MultiMesh `fly_agent.glb` instances oriented along velocity vector; God Fly distinct gold model. | Needs visual behavior state presentation: flying, foraging, resting, feeding, wing-beat animation cues, and color coding by colony/species. |
| **Map & Minimap System** | `MISSING` | No interactive map or minimap exists in the Godot frontend. | Implement synchronized Minimap / World Map layer with terrain elevation, water, colony markers, agent density, and toggleable data layers. |
| **Atmosphere & Diurnal Cycle** | `PARTIAL` | Static ProceduralSky, DirectionalLight3D sun, weather fog attenuation, and GPU precipitation particles. | Needs diurnal sun movement (dawn, noon, golden hour, sunset, night), sky color progression, and weather-driven illumination. |
| **Cinematic Broadcast Engine** | `PARTIAL` | 4-Camera QuadView presentation with SubViewports; hotkeys 0-4 for solo switching; channel badges. | Needs autonomous broadcast switcher: automatic event-driven cut transitions, dynamic shot selection, lower-third event ticker, and broadcast controls. |
| **Replay & Checkpoint Recovery** | `VERIFIED` | C++ bit-exact state checkpoint restore (`--checkpoint`, `--restore`, `--replay`). | Expose visual state replay verification in Godot test suite. |
| **Release Packaging & Installer** | `VERIFIED` | Automated packaging script produces portable ZIP/tar.gz and native C# Windows installer `FLGODTV-0.1.0-Setup.exe`. | Update packaging with new assets, shaders, and test verification report. |

---

## 3. Concrete Visual V2 Implementation Plan

### Step 1: C++ Event System & State Exporter Upgrade
- Extend `SimulationEventType` in `include/flgod/camera/event_detector.hpp` with all Visual V2 event categories (`Combat`, `Predation`, `Discovery`, `ResourceDiscovery`, `Construction`, `Migration`, `Fire`, `Flooding`, `UnusualBehavior`).
- In `src/main.cpp`, enhance `export_live_state()` to export:
  - Canonical 2D world fields summary (temperature, moisture, ecology, elevation grid).
  - Complete agent states (`action`, `hunger`, `energy`, `species_id`, `state_name`).
  - Active event list with world coordinates.

### Step 2: Terrain V2 & Biome Visualization
- Enhance `godot/scripts/terrain_renderer.gd` with:
  - Multi-texture or slope-aware vertex shader/material blending: steep slopes render as rock/cliff, low flat land near water as sand/shore, plains as lush grass, higher altitudes as moss/alpine rock, and summits as snow.
  - Compute accurate vertex normals and biome mask weights from canonical elevation and slope.
  - Seamless chunk LOD transitions and distance culling.

### Step 3: Water V2 Presentation
- Enhance `godot/scripts/water_renderer.gd` with:
  - Depth-based transparency and color gradient (shallow turquoise shoreline to deep ocean blue).
  - Procedural shoreline foam fringe using depth/distance comparison against terrain height.
  - Directional flow ripples and Fresnel surface reflection approximation.

### Step 4: Vegetation & Environmental Richness (Vegetation V2)
- Enhance `godot/scripts/vegetation_renderer.gd`:
  - Multi-class environmental instancing: tall trees, flowering shrubs, short grass tufts, dead branches, and scattered boulders.
  - Distribute instances based on real slope, elevation, and moisture conditions (trees in fertile valleys, rocks on slopes, flowers in meadows).
  - Use GPU MultiMesh with color variation and wind-sway animation.

### Step 5: Agent Visuals & Animations (Agent V2)
- Enhance `godot/scripts/flgod_tv_controller.gd`:
  - Agent state machine visualization: altitude and speed modulation reflecting actions (`Forage`, `Fly`, `Hover`, `Rest`, `Interact`).
  - Wing oscillation / banking on turns.
  - Colony and species distinguishing color schemes and scale variations.

### Step 6: World Map & Minimap System
- Implement `godot/scripts/map_system.gd`:
  - 2D CanvasLayer radar minimap in upper-right corner synchronized with world coordinate space `[-45, 105]`.
  - Displays real terrain elevation contour, water shoreline, Colony 1 & 2 nests, God Fly beacon, and moving agent dots.
  - Minimap layer toggle (M key) and layer filter controls (Colonies, Agents, Food, Events).

### Step 7: Atmosphere, Diurnal Lighting & Weather V2
- Implement `godot/scripts/atmosphere_controller.gd`:
  - Time-of-day progression driven by simulation clock: Dawn (warm pink/orange), Noon (bright daylight), Golden Hour (warm amber), Sunset (deep violet/red), Night (deep blue moonlight).
  - Synchronize DirectionalLight3D rotation, energy, and sky colors.
  - Dynamic weather coupling: overcast dimming, storm precipitation, and horizon mist.

### Step 8: Autonomous FLGODTV Broadcast Engine
- Enhance `godot/scripts/flgod_tv_controller.gd`:
  - Autonomous director mode: monitors high-priority simulation events and automatically switches the active broadcast channel / solo view with cinematic hold timers and hysteresis.
  - Broadcast lower-third banner displaying breaking event notices, agent focus, and connectome telemetry.

### Step 9: Testing, Evidence & Verification
- Create automated visual verification test script `godot/scripts/test_visual_v2.gd`.
- Generate and archive baseline and upgraded evidence under `evidence/visual_v2/`:
  - Baseline execution evidence.
  - Visual V2 screenshots: Full World Horizon, Biome Blending, Water Shoreline, Vegetation Richness, Minimap HUD, 4-Camera Broadcast, Diurnal Cycle.
- Run complete 52+ test CTest suite and ensure 100% green execution.
- Update release packages, SHA-256 checksums, documentation, and push to GitHub.
