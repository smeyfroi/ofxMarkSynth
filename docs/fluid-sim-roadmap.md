# Fluid Simulation Roadmap (ofxRenderer + ofxMarkSynth)

Note: deferred/backlog items live in `docs/future-work.md`.

This document started as a staged plan for improving the fluid simulation used by `ofxMarkSynth`’s `FluidMod` (which wraps `ofxRenderer`’s `FluidSimulation`).

It now also serves as a running record of what has been implemented, plus a short list of explicitly deferred items ("for later").

## Defaults / Policy

- Default boundary mode: `SolidWalls`.
- Wrap mismatch: **hard error** (disable sim and log one actionable message).
- External-FBO support stays first-class: `FluidSimulation` must accept `PingPongFbo`s passed in from MarkSynth drawing layers.
- Manual testing: changes are evaluated by running a fixed set of configs and noting qualitative deltas (edges, flow scale, swirl, persistence, stability). Automated testing is not a goal.

## Status (implemented)

High-level implementation record (as of 2026-01):

- `ofxRenderer/example_fluid` is a working debug harness:
  - Debug draw modes include divergence/pressure/curl/temperature/obstacles.
  - Reset buttons exist for temperature/obstacles.
- `FluidSimulation` is strict and introspectable:
  - Validity state (`isSetup()`/`isValid()`), actionable validation errors.
  - Boundary mode validation against wrap mode.
  - Debug getters: divergence/pressure/curl/temperature/obstacles.
  - `dt` parameter range clamped to `0.0–0.003`.
- Impulse system upgraded:
  - Directional injection (`PointVelocity`/`Velocity`) and optional swirl controls.
  - Normalized/portable semantics (displacement-per-step feel).
- Buoyancy + temperature implemented:
  - Buoyancy v1 (dye-driven) and temperature v2 (separate field) both available.
  - `TempEnabled` leaf param (renamed from `Enabled` to avoid flattening ambiguity).
- Obstacles implemented:
  - Optional `layers.obstacles` input supported (mask sampled as `max(alpha, luma(rgb))`).
  - Obstacle-aware advection/diffusion/divergence/projection + masking for impulses/vorticity/buoyancy.
  - Known issue: obstacle-edge dye bleed can still occur; we chose not to add the heavier “rigid guard” passes yet.
- MarkSynth integration hardened:
  - `FluidMod` logs-once and skips update when the underlying sim is invalid.
  - `FluidMod` supports temperature injection sinks (`TempImpulsePoint/Radius/Delta`) + `TempEnabled`.
  - `FluidMod` supports optional `layers.obstacles` forwarding to the sim.
- Config validator upgraded:
  - Hard errors for `Fluid.dt` vs `FluidRadialImpulse.dt` mismatches.
  - Hard errors for boundary-mode ↔ wrap mismatches (now includes optional obstacles layers).
- Config migrations executed in MarkSynth-performances repos:
  - Improvisation1: grid-wide param normalization + dt halving pass; docs updated with dt range learnings.
  - CaldewRiver: migrated to SolidWalls/clamp, dt scaling (`dt/30` when legacy), spreads defaults, impulse dt sync; docs updated.

For later (explicitly deferred):
- BFECC/MacCormack advection (quality vs performance).
- Optional “rigid obstacle guard” passes to fully eliminate bleed (extra fullscreen passes; costly for large values buffers).
- MarkSynth debug sources for divergence/pressure/curl as explicit `FluidMod` emitted sources (currently velocities only).
- Replace string-based parameter grabs in `FluidMod` with typed accessors (current approach is consistent with other mods; defer).

## External Surface Additions (migration-relevant)

This section intentionally lists the externally visible controls (parameters/sinks) that need to be considered when migrating older configs.

- `FluidMod` / `FluidSimulation` parameters:
  - `Boundary Mode` (0=SolidWalls, 1=Wrap, 2=Open)
  - Core: `dt`, `Vorticity`, `Value Dissipation`, `Velocity Dissipation`, `Value Spread`, `Velocity Spread`, `Value Max`
  - `Temperature`: `TempEnabled`, `Temperature Dissipation`, `Temperature Spread`, `Temperature Iterations`
  - `Buoyancy`: `Buoyancy Strength`, `Buoyancy Density Scale`, `Buoyancy Threshold`, `Gravity Force X`, `Gravity Force Y`, `Use Temperature`, `Ambient Temperature`, `Temperature Threshold`
  - `Obstacles`: `ObstaclesEnabled`, `Obstacle Threshold`, `Obstacle Invert`
- `FluidMod` layers:
  - Optional: `layers.obstacles` (mask layer sampled as `max(alpha, luma(rgb))`; must match velocity resolution + boundary-mode wrap)

- `FluidMod` sinks:
  - `TempImpulsePoint` (vec2), `TempImpulseRadius` (float), `TempImpulseDelta` (float)
- `FluidRadialImpulseMod` (new impulse surface):
  - Directional injection sinks: `PointVelocity`, `Velocity`
  - Optional artistic control: `SwirlVelocity` sink + `SwirlStrength` parameter (useful for cold-start curl / vortex seeds)
  - Parameters: `VelocityScale`, `SwirlStrength`, `SwirlVelocity`

## Phase 1 — Make `ofxRenderer/example_fluid` a debugging reference

Status: implemented.

Goal: quickly see what each pipeline change does, without MarkSynth complexity.

- Add debug draw modes:
  - Dye/values
  - Velocity magnitude
  - Divergence
  - Pressure
  - Curl/vorticity
- Add a reproducible driver (no mouse dependency):
  - Constant drift and/or periodic impulse injection
- Add on-screen sim info overlay:
  - FBO sizes, internal formats
  - Wrap modes
  - `tex_u/tex_t`
  - Texture target

Files:
- `ofxRenderer/example_fluid/src/ofApp.cpp`
- `ofxRenderer/example_fluid/src/ofApp.h`

Manual check:
- Reproduce the margin/top-right dead band quickly and consistently.

## Phase 2 — Strict validation + debug outputs in `FluidSimulation`

Status: implemented.

Goal: fail fast instead of producing mysterious artifacts.

- Add validation that runs on setup and logs once:
  - allocated, non-zero sizes
  - normalized UV domain requirement (`tex_u == 1` and `tex_t == 1`)
  - texture target compatible with `sampler2D`
  - formats are sane (warn vs error as needed)
  - values/velocities size mismatch (warn initially)
- Add boundary-mode/wrap validation (**hard error**):
  - `SolidWalls` requires `GL_CLAMP_TO_EDGE` on all sampled fields (values, velocities; later temperature/obstacles).
  - `Wrap` requires `GL_REPEAT`.
  - `Open` should be defined explicitly (likely clamp).
- Add explicit validity state:
  - `isSetup()` vs `isValid()`
  - `update()` becomes a no-op when invalid.
- Expose debug textures/getters for diagnostics:
  - divergence texture
  - pressure texture
  - curl texture

Files:
- `ofxRenderer/src/fluid/FluidSimulation.h`

Manual check:
- Intentionally misconfigure wrap/ARB/tex_u and confirm it hard-errors with an actionable message.

## Phase 3 — Fix edge/margin defect by making UV domain explicit in shaders

Status: deferred (current implementation relies on normalized UVs and strict texture validation).

Goal: eliminate dependence on implicit `texCoordVarying` mapping.

- Change all fluid passes to compute UV from pixel coordinates:
  - `uv = (gl_FragCoord.xy + 0.5) / texSize`
- Ensure neighbor offsets are consistent (`off = vec2(1,0) / texSize`).

Files:
- `ofxRenderer/src/fluid/AdvectShader.h`
- `ofxRenderer/src/fluid/JacobiShader.h`
- `ofxRenderer/src/fluid/DivergenceRenderer.h`
- `ofxRenderer/src/fluid/SubtractDivergenceShader.h`
- `ofxRenderer/src/fluid/VorticityRenderer.h`
- `ofxRenderer/src/fluid/ApplyVorticityForceShader.h`

Manual check:
- The ~10% top/right margin disappears in `example_fluid` with clamp + SolidWalls.

## Phase 4 — Implement explicit boundary mode (start with SolidWalls)

Status: implemented.

Goal: boundary behavior becomes intentional.

- Add `BoundaryMode` parameter:
  - `SolidWalls`, `Wrap`, `Open`
- Implement velocity boundary enforcement for `SolidWalls`:
  - zero normal velocity at edges
  - handle corners explicitly if needed
- Apply boundary enforcement at stable points:
  - after advection/diffusion
  - after projection

Files:
- `ofxRenderer/src/fluid/FluidSimulation.h`
- new boundary shader header under `ofxRenderer/src/fluid/`

Manual check:
- With drift, dye/velocity respects walls and no longer produces unexplained dead bands.

## Phase 5 — Correct core math (hybrid: correct backbone, artist knobs)

Status: mostly implemented (the remaining quality work is explicitly deferred).

Goal: make the solver more portable (resolution/FPS) while keeping the UI/configs artist-friendly.

Key decisions:
- Baseline configs assume ~30 FPS (MarkSynth performance target).
- `Fluid.dt` is a dimensionless *Sim Speed* scalar tuned around 30fps; internal solver uses `dtEffective = ofGetLastFrameTime() * 30 * dt`.
- `Vorticity` is treated as a normalized 0..1 *Swirl* control and mapped internally (currently `0..1 -> 0..0.3`).
- `Value Dissipation` / `Velocity Dissipation` are normalized *persistence* knobs (0..1) mapped internally to half-life seconds (separate ranges for value vs velocity).
  - Keep them exposed for now (despite the legacy names); they remain key controls for how long dye/motion persists.
  - Rename to `Value Persistence` / `Velocity Persistence` during config migration, not before.
- We will not hand-tune configs; instead we will derive a mechanical translation heuristic to migrate configs once the implementation stabilizes.

Backbone correctness work:
- Introduce explicit `dx` (cell size) semantics:
  - divergence computation
  - pressure gradient subtraction
  - pressure solve alpha
- Fix Jacobi diffusion parameterization:
  - remove constant `rBeta=0.25` assumption
  - compute `rBeta = 1 / (4 + alpha)` consistently
  - diffusion must use a stable `b` (snapshot the field once per solve)
- Add stability clamps where needed:
  - CFL-style clamp for injected impulses
  - CFL-style clamp after vorticity confinement (to prevent runaway)
  - optional `Value Max` clamp after advection to prevent dye blowout
- Startup responsiveness:
  - purely radial impulses tend to be projected away at cold start (divergence-heavy, curl-light)
  - add a tangential/swirl impulse option so curl is visible from frame 1
- Advection quality (later):
  - BFECC / MacCormack advection can preserve contrast/tendrils better than semi-Lagrangian (reduces the need to "compensate" with higher mark alpha)
  - too GPU-expensive at current MarkSynth resolutions (e.g. values ~2400^2) to implement now; revisit after performance budget review

Files:
- `ofxRenderer/src/fluid/FluidSimulation.h`
- `ofxRenderer/src/fluid/JacobiShader.h`
- `ofxRenderer/src/fluid/DivergenceRenderer.h`
- `ofxRenderer/src/fluid/SubtractDivergenceShader.h`

Manual check:
- Divergence debug view trends closer to zero post-projection.
- Flow scale doesn’t change wildly when sim resolution changes.

## Phase 6 — Update MarkSynth wrapper (`FluidMod`) to survive sim evolution

Status: partially implemented (validation + temp impulses + obstacles forwarding done; typed parameter accessors and debug sources deferred).

Goal: keep MarkSynth integration stable while the sim’s parameters and outputs grow.

- Replace string-based parameter grabs in `FluidMod` with typed accessors on `FluidSimulation`:
  - today it depends on exact names like `"Value Dissipation"`.
  - add `FluidSimulation::dtParam()`, `vorticityParam()`, etc., and bind controllers to those.
- Mirror strict validation behavior (CollageMod-style):
  - if invalid: log error and skip update.
- Expand `FluidMod` sources for debugging/downstream usage:
  - velocities (existing)
  - divergence/pressure/curl (new) — consider exposing as named layers (e.g. `fluid_divergence`, `fluid_pressure`, `fluid_curl`) so they can be inspected/recorded
 
Files:
- `ofxMarkSynth/src/layerMods/FluidMod.cpp`
- `ofxMarkSynth/src/layerMods/FluidMod.hpp`
- `ofxRenderer/src/fluid/FluidSimulation.h`

Manual check:
- MarkSynth configs that use `GL_REPEAT` for fluid fields hard-error with a clear fix instruction.

## Phase 7 — Temperature + buoyancy (new functionality after stability)

Status: implemented (v1 dye buoyancy + v2 temperature field + MarkSynth injection sinks).

### Phase 7a — Buoyancy v1 (dye-driven)

Status: implemented.

- Implement a modern `#version 410` buoyancy pass and apply it before projection.
- Use the existing dye/values field as “density” (no temperature buffer yet): `density = max(alpha, luma(rgb))`.
- Parameters live under `Fluid Simulation > Buoyancy`:
  - `Buoyancy Strength`, `Buoyancy Density Scale`, `Buoyancy Threshold`, `Gravity Force X/Y`

Artistic intent (MarkSynth): makes existing marks/dye behave like “smoke/plumes” without adding a new injection mod.

### Phase 7b — Temperature v2 (separate scalar field)

Goal: decouple “visual dye” from “thermal force” so we can drive motion without brightening marks.

- Add a dedicated temperature ping-pong scalar field (likely single-channel float) + parameters:
  - `Temperature Dissipation` (persistence)
  - `Ambient Temperature`
  - `Buoyancy Strength` (now based on `temperature - ambient`)
  - Optional: `Temperature Diffusion` (low-iter smoothing)
- Temperature pipeline per step:
  - Advect temperature by velocity
  - Apply dissipation / optional diffusion
  - Apply buoyancy force from temperature
  - Project velocity
- Add temperature injection path in `example_fluid` (mouse/key driven, separate from dye injection).
- MarkSynth: postpone integration until there is a clean pattern for “external parameter fields” from Mods.

Files:
- `ofxRenderer/src/fluid/FluidSimulation.h`
- `ofxRenderer/src/fluid/ApplyBouyancyShader.h`
- (new) temperature advect/diffuse shader or reuse `AdvectShader` with scalar target

## Phase 8 — Obstacles/masks (later)

Status: implemented (with a known remaining edge-bleed issue; rigid guard deferred).

- Add obstacle mask field (sampled as `max(alpha, luma(rgb))`).
- Make advection/diffusion/divergence/projection obstacle-aware; optionally mask vorticity/buoyancy/impulses.
- MarkSynth integration: allow passing an arbitrary drawing layer as `layers.obstacles` (must match velocity resolution and boundary-mode wrap).
- Potential future refinement: add an optional “rigid obstacle guard” pass that forcibly clears fields inside solid cells each frame (prevents dye/temperature bleed at boundaries, but costs extra fullscreen passes—especially expensive for very large values buffers).

## Manual backtest loop (per phase)

At the end of each phase (especially 2–6):

- Run the manual test set (see `fluid-backtest-configs.txt` in the MarkSynth-performances folder).
- Note qualitative deltas: edge behavior, flow scale, swirl intensity, persistence, stability.

## Config migration guide (one pass)

This is the “mechanical” porting checklist for converting legacy fluid configs to the current external surface. It focuses on correctness/compatibility first; treat aesthetic compensation as a second pass.

### Required (functional)

- **Wrap + boundary mode**
  - If values/velocities layers use `GL_CLAMP_TO_EDGE`, set `Fluid.Boundary Mode = 0` (SolidWalls).
  - If they use `GL_REPEAT`, set `Fluid.Boundary Mode = 1` (Wrap).
  - Avoid mixed wrap across values/velocities; the sim validates this strictly.

- **Velocity layer exists and is compatible**
  - Ensure there is a `velocities` drawing layer and that Fluid binds it via `layers.velocities`.
  - Recommended for the velocities layer: `GL_RG32F`, `OF_BLENDMODE_DISABLED`, `GL_CLAMP_TO_EDGE`, `isDrawn=false`.

- **dt consistency (do not auto-sync in code; do sync in configs)**
  - For every config, make `Fluid.dt` equal to all `FluidRadialImpulse.dt` values that draw into the same velocities layer.
  - If migrating from a legacy dt convention, a starting point is: `dt_new ≈ dt_old / 30`.

- **Directional injection (optional functional upgrade)**
  - If you have a flow source that emits point velocity (e.g. camera motion), you may connect:
    - `VideoFlowSourceMod.PointVelocity -> FluidRadialImpulseMod.PointVelocity`
  - If you do not add this connection, point-only impulses still work (radial only).

### Optional (artist controls)

- **Swirl impulses (optional artistic control)**
  - Use `FluidRadialImpulseMod.SwirlVelocity` (sink or parameter) with `SwirlStrength` as a multiplier.
  - Purpose: add immediate curl/vortex “seeding” and keep cold-start flow visible.

- **Buoyancy + temperature (new feature surface)**
  - Buoyancy v1 (dye-driven): enable `Buoyancy Strength` and keep `Use Temperature = 0`.
  - Buoyancy v2 (temperature-driven): set `TempEnabled = 1`, `Use Temperature = 1`, then inject via `FluidMod` temp sinks (`TempImpulsePoint/Radius/Delta`).

### Likely requires an artistic pass (do not try to fully automate)

- Mark alpha / fade compensation (e.g. `AlphaMultiplier`, `ground.alpha`, `Fade.Alpha` mappings).
- Dissipation semantics: numeric keys are still named `Value Dissipation` / `Velocity Dissipation`, but they now behave as normalized persistence controls; expect to retune by eye.
