# Video motion tuning plan (venue calibration)

Goal: tune MarkSynth’s **video-based movement detection** for a specific venue / performer / camera placement.

The focus is **gesture sensitivity** (ignore subtle sway/breathing), independent of audio/musical events.

## Scope (v1)

- One camera / one `VideoFlowSourceMod`.
- Primary tuning target is the **derived motion scalars** (e.g. motion magnitude envelopes), with the flow texture as a secondary visual sanity check / field driver.
- No ROI masking, mirroring/rotation, or motion onset events in v1.

## Current pipeline

1) `VideoFlowSourceMod` provides:
- `FlowField` texture (GPU-only, used as a field by downstream mods)
- `PointVelocity` samples (CPU sampling from flow texture)

2) `MotionFromVideo` (from `ofxMotionFromVideo`) computes optical flow:
- Shader parameters: `offset`, `threshold`, `force`, `power`

3) Motion scalar extraction:
- `VectorMagnitudeMod` (used to turn motion vectors into stable 0–1 envelopes)

## Key v1 parameters

### Sampling (gesture gate)
- `MinSpeedMagnitude`
  - single scalar gate controlling what counts as “motion”
  - higher values ignore subtle motion

### Sampling rate
- `PointSamplesPerUpdate`
  - number of random points sampled per frame

### Optical flow shader (secondary)
- `offset` (spatial comparison distance)
- `threshold` (noise floor)
- `force` (overall flow strength)
- `power` (nonlinear emphasis/suppression of subtle motion)

### Motion magnitude scalars (primary)

`VectorMagnitudeMod` is typically used to turn camera motion vectors into stable 0–1 control envelopes.

Parameters commonly tuned:
- `Components`
- `Min`, `Max` (normalisation)
- `MeanSmoothing`, `MaxSmoothing`, `DecayWhenNoInput`

## Performance considerations

- The expensive GPU→CPU readback (`readToPixels`) is only enabled when point sampling is actually required:
  - when `PointSamplesPerUpdate > 0` AND there is at least one connection from `VideoFlowSourceMod`’s `PointVelocity` or `Point` source.
- Video debug drawing from `VideoFlowSourceMod::draw()` is removed; textures are shown only inside the ImGui Debug View Video tab.

## UI (ImGui)

A new Debug View tab:
- `Debug View → Video`

Includes:
- `MinSpeedMagnitude` + `PointSamplesPerUpdate`
- live sampling stats (accepted/attempted, mean/max accepted speed)
- optical flow controls (offset/threshold/force/power)
- motion scalar inspector for motion-magnitude `VectorMagnitudeMod` instances
- optional texture previews (video + flow)

## Presets, snapshots, and overrides

This project needs two scopes:

- **Performance scope** (shared across all configs in a performance folder)
- **Config scope** (data specific to one synth config, identified by its filename stem)

We intentionally separate these concerns to avoid accidental coupling between:
- stable config factoring (presets)
- expressive performance recall (snapshots)

### Performance-level presets (automatic)

Presets are shared parameter blocks used to factor out repeated config values.

- File: `performanceConfigRootPath/mod-params/presets.json`
- Schema (see `docs/venue-and-mod-presets-plan.md`):
  - `{ type: { "_default": { paramName: valueString, ... }, presetName: { ... } } }`
- Mod instances reference a preset using a mod-level key:
  - `"preset": "PresetName"`

Application order (important):
- Venue + preset defaults are applied **before** parameter defaults are captured.
- This prevents venue/preset defaults being re-saved into synth configs.

### Config-level snapshots (manual, on-demand)

Snapshots are a runtime performance tool (slot recall + undo). They are not loaded automatically.

- Identifier: `configId = fileStem(currentConfigPath)` (e.g. `31-movement4-meander-base`)
- File: `performanceConfigRootPath/mod-params/snapshots/<configId>.json`
- Structure:
  - `snapshots[slotIndex] = { name, timestamp, mods: { modName: { paramName: valueString } } }`

Behavior:
- The snapshot UI loads the current config’s snapshot file lazily (only when saving/loading a slot).
- Snapshot files are shared across all synths using the same performance folder (scoped by configId only).

### Venue presets (venue calibration)

Venue calibration defaults live in:

- File: `performanceConfigRootPath/venue-presets.json`

See `docs/venue-and-mod-presets-plan.md` for schema and precedence.

## Stripping per-config tuning

Per-config files should remain clean: venue + preset defaults belong in the two performance-scoped preset files.

## Future work

See `docs/future-work.md` (Video Motion Tuning).

---

## Amended plan: unify venue defaults + presets (audio + video)

See `docs/venue-and-mod-presets-plan.md`.
