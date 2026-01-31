# Future Work (Deferred / Out Of Scope)

This file centralizes “future”, “phase 2+”, and “out of scope” notes found across `docs/`.
It is intended to be the single clean backlog list, while the individual plan docs stay focused on current scope.

## Video Motion Tuning

Source: `docs/video-motion-tuning-plan.md`

- ROI masking / crop (ignore audience/lighting movement)
- Mirroring / rotation controls (camera/projector rig differences)
- Motion onset events (gesture triggers)
- Multi-camera support:
  - multiple `VideoFlowSourceMod` instances
  - per-camera presets

## Audio Tuning Inspector

Source: `docs/audio-tuning-inspector-plan.md`

- Phase 2: 30s capture + tuning suggestions
  - capture percentiles per scalar (`p05/p50/p95`)
  - suggested effective min/max per scalar (real units)
  - suggested multiplier deltas relative to currently-loaded config
- Next after Inspector:
  - Synth-level “venue tuning” parameters (persistent across config switches) that apply as multipliers/offsets
  - MIDI encoder bindings (Launch Control XL 3) for venue calibration

## Venue + Mod Presets Unification

Source: `docs/venue-and-mod-presets-plan.md`

- Optional: GUI tooling for editing/saving `venue-presets.json` and `mod-params/presets.json`
- Optional: support multiple venue profiles
- Optional: add schema/versioning for safety

## Agency System

Source: `docs/agency-system-plan.md`

- Memory bank event gating (`MemoryEmit*`) — deferred
- TextSource event gating (`TextSource.Next`) — deferred
- Separate audio-vs-video agency lanes — deferred
- Re-introducing Smear reset/disable — defer until explicit sink names rather than threshold bands

## Wrapper Param Controllers

Source: `docs/wrapper-param-controllers.md`

- Phase 2: update wrapper Mods to apply overrides
  - create controllers bound to manual params
  - update controllers per-frame, build overrides, apply overrides only when changed
  - remove programmatic writes to manual `ofParameter` used only to push effective values

## Fluid Simulation

Source: `docs/fluid-sim-roadmap.md`

- Advection quality (later): BFECC/MacCormack for better contrast/tendrils (needs perf budget review)
- Optional rigid obstacle guard passes to eliminate edge bleed (extra fullscreen passes; costly)
- MarkSynth debug sources for divergence/pressure/curl as explicit `FluidMod` emitted sources
- Replace string-based parameter grabs in `FluidMod` with typed accessors on `FluidSimulation`
- Phase 3 (deferred): fix edge/margin defect by making UV domain explicit in shaders
  - compute UV from `gl_FragCoord` + texture size
  - ensure neighbor offsets are consistent
