# Fluid Simulation Roadmap (ofxRenderer + ofxMarkSynth)

This document captures a staged plan for improving the fluid simulation used by `ofxMarkSynth`’s `FluidMod` (which wraps `ofxRenderer`’s `FluidSimulation`).

The work is intentionally paced over multiple sessions: fix defects first, then add correctness, then add new functionality (temperature/buoyancy/obstacles).

## Defaults / Policy

- Default boundary mode: `SolidWalls`.
- Wrap mismatch: **hard error** (disable sim and log one actionable message).
- External-FBO support stays first-class: `FluidSimulation` must accept `PingPongFbo`s passed in from MarkSynth drawing layers.
- Manual testing: changes are evaluated by running a fixed set of configs and noting qualitative deltas (edges, flow scale, swirl, persistence, stability). Automated testing is not a goal.

## Phase 1 — Make `ofxRenderer/example_fluid` a debugging reference

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

Files:
- `ofxRenderer/src/fluid/FluidSimulation.h`
- `ofxRenderer/src/fluid/JacobiShader.h`
- `ofxRenderer/src/fluid/DivergenceRenderer.h`
- `ofxRenderer/src/fluid/SubtractDivergenceShader.h`

Manual check:
- Divergence debug view trends closer to zero post-projection.
- Flow scale doesn’t change wildly when sim resolution changes.

## Phase 6 — Update MarkSynth wrapper (`FluidMod`) to survive sim evolution

Goal: keep MarkSynth integration stable while the sim’s parameters and outputs grow.

- Replace string-based parameter grabs in `FluidMod` with typed accessors on `FluidSimulation`:
  - today it depends on exact names like `"Value Dissipation"`.
  - add `FluidSimulation::dtParam()`, `vorticityParam()`, etc., and bind controllers to those.
- Mirror strict validation behavior (CollageMod-style):
  - if invalid: log error and skip update.
- Expand `FluidMod` sources for debugging/downstream usage:
  - velocities (existing)
  - divergence/pressure/curl (new)

Files:
- `ofxMarkSynth/src/layerMods/FluidMod.cpp`
- `ofxMarkSynth/src/layerMods/FluidMod.hpp`
- `ofxRenderer/src/fluid/FluidSimulation.h`

Manual check:
- MarkSynth configs that use `GL_REPEAT` for fluid fields hard-error with a clear fix instruction.

## Phase 7 — Temperature + buoyancy (new functionality after stability)

Goal: implement temperature field correctly (and fix shader version mismatch).

- Add temperature ping-pong scalar field + parameters.
- Modernize `ApplyBouyancyShader` to match `#version 410` pipeline (currently uses legacy `gl_TexCoord/gl_FragColor`).
- Add temperature injection path in `example_fluid`.
- Optionally add MarkSynth temperature injection mod later.

Files:
- `ofxRenderer/src/fluid/FluidSimulation.h`
- `ofxRenderer/src/fluid/ApplyBouyancyShader.h`

## Phase 8 — Obstacles/masks (later)

- Add obstacle mask field.
- Make divergence/projection/advection obstacle-aware.
- Add MarkSynth integration via an `obstacles` layer key and an injection/drawing mod.

## Manual backtest loop (per phase)

At the end of each phase (especially 2–6):

- Run the manual test set (see `fluid-backtest-configs.txt` in the MarkSynth-performances folder).
- Note qualitative deltas:
  - edge behavior
  - flow scale
  - swirl intensity
  - persistence
  - stability
- Iterate on a small set of migration heuristics (rules of thumb), starting with:
  - `dt` translation: **tentative** (old sim used `dtEffective = frameDt * dt_old`, new uses `dtEffective = frameDt * 30 * dt_new`): `dt_new ≈ dt_old / 30`
  - Dissipation translation: old per-frame dissipation `d` -> half-life at 30fps -> invert persistence mapping
    - half-life seconds: `halfLife = (1/30) * ln(0.5) / ln(d)`
  - Impulse translation: old radial impulse multiplied by `dt` (acceleration-like); new impulses are specified as **pixels of desired displacement per step** (vector + radial + swirl). Internally these are resolution-normalized and divided by `dtEffective` to become UV velocity.
    - TODO: Update MarkSynth `RadialImpulseMod` to accept both `position` and a `velocity` vector (so it can drive the new directional impulse path)
  - Pressure iteration translation: map iterations to a 0..1 quality knob (once implemented)
    - High-res budget note (e.g. MarkSynth `~2400x2400`): start with `Pressure≈10`, `Velocity diffusion≈1`, `Value diffusion≈1` and adjust using divergence view
  - Preserve dye injection when radius changes: keep `points * radius^2 * alpha` approximately constant
