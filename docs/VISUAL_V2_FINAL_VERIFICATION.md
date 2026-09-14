# Visual V2 Final Verification (Current State)

**Supersedes the pre-implementation baseline in `docs/VISUAL_V2_AUDIT.md`, which is
preserved unchanged as the historical baseline. This document describes the
CURRENT verified runtime state.**

- **Commit:** `53384aabb444885100bde45e207b5ef5b8669af5`
- **Date:** 2026-09-14
- **Renderer:** Godot 4.7.2 Forward+ Vulkan, AMD Radeon(TM) Graphics, Vulkan 1.4.315
- **Method:** windowed CTest targets (`GodotFrontendSmokeTest`,
  `GodotVisualV2VerificationTest`); captures taken from real rendered frames via
  `RenderingServer.frame_post_draw` after 30 frames; stale files always
  overwritten; empty/uniform frames FAIL (no synthetic fallback).

## Subsystem results (all values generated from execution)

| Subsystem | Result | Metric |
|---|---|---|
| Terrain V2 | PASS | 13,254 vertices, slope-aware biomes, LOD0/1/2, streaming, backend grid |
| Water V2 | PASS | dual-plane surface + foam fringe, backend level sync |
| Vegetation V2 | PASS | 118 flora instances, 22 rock boulders, backend mirroring |
| Agent V2 | PASS | MultiMesh fly renderer, God Fly gold model, Blender glTF assets |
| Map system | PASS | world-to-map projection, layer cycling |
| Atmosphere | PASS | Dawn/Noon/Sunset/Night diurnal progression |
| Broadcast | PASS | autonomous director hold timer 8.0 s, 4-camera QuadView |
| Telemetry HUD | PASS | 7 panels, live binding, strict N/A fallback, toggles |

## Artifacts

- Screenshots: `evidence/visual_v2/screenshots/` (6 PNGs, 1370x749, fresh renders)
- Manifest: `evidence/visual_v2/manifest.json` (counts from execution)
- Objective validation: `evidence/visual_v2/visual_validation.json` — per artifact:
  dimensions, SHA-256, sampled unique colors, pixel variance, PASS/FAIL.
- UI/UX capture: `renders/screenshots/ui_ux_headless_verification.png`
  (1370x749 real render; the previous 5 KB flat-fill fallback was removed).

## Known limitations

- Captures require a display + Vulkan GPU (windowed run). `--headless` uses the
  dummy rasterizer and yields no pixels; cloud CI reports Godot as NOT_AVAILABLE.
- Capture resolution follows the OS window size (1370x749 here), not 1280x720.
