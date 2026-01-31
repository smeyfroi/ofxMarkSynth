# Config Defaults + Presets Guide (Venue → Performance → Config)

This document describes the MarkSynth “hierarchical defaults” scheme used to:

- keep synth config files small and readable
- avoid repeated parameter blocks across many configs
- centralize venue calibration
- ensure that performance-scoped defaults do **not** get re-saved into per-config files

It is intended as a practical guide for refactoring existing performance folders and for authoring new config sets.

## Goals

- **Centralize** stable parameter sets (venue tuning, reusable presets).
- **Localize** only truly unique parameters in each synth config.
- Make it easy to answer: “where is this value defined?”
- Keep config saving clean: defaults should not be written back into configs.

## Node Editor cues

The node editor tries to make the defaults/presets hierarchy visible while patching.

- **Node titles**: `ModName [PresetName]` is shown when a Mod has an explicit `"preset"` (excluding `_default`).
- **Dimmed parameter labels**: value matches the Mod’s captured defaults (after applying `venue-presets.json` + `mod-params/presets.json`).
- **Normal parameter labels**: value differs from defaults (typically from per-config `mods.<ModName>.config`, snapshots, or live control).

## The hierarchy (“levels”)

MarkSynth uses two performance-scoped preset files plus per-config overrides.

### Level 0 — Code defaults

Every Mod defines base defaults in C++ (`ofParameter` constructor values).

### Level 1 — Venue defaults (`venue-presets.json`)

Venue calibration values (mic gain, room lighting, camera placement) live in:

- `performanceConfigRootPath/venue-presets.json`

This file is shared across all synth configs in that performance folder.

### Level 2 — Performance presets (`mod-params/presets.json`)

Reusable per-type preset blocks live in:

- `performanceConfigRootPath/mod-params/presets.json`

This file is also shared across all synth configs in that performance folder.

### Level 3 — Per-config overrides (synth config JSON)

Each synth config may still include a per-mod `config` object for unique values:

- `performanceConfigRootPath/synth/<config>.json`
- `mods.<ModName>.config` contains only parameters unique to that config.

### Level 4 — Runtime state (snapshots, UI)

Runtime state is intentionally separate:

- snapshots are stored in `performanceConfigRootPath/mod-params/snapshots/<configId>.json`

Snapshots are for performance recall/undo and are not part of the default hierarchy.

## JSON schema (venue + mod presets)

Both `venue-presets.json` and `mod-params/presets.json` use the same schema.

- Top-level keys are Mod `type` strings (factory types), e.g. `VideoFlowSource`, `Fluid`, `MultiplyAdd`.
- Each type contains named blocks:
  - `_default`: the baseline for that type
  - `<PresetName>`: additional preset blocks selected by `mods.<ModName>.preset`

Example:

```json
{
  "VideoFlowSource": {
    "_default": {
      "MinSpeedMagnitude": "0.4",
      "PointSamplesPerUpdate": "140"
    },
    "CameraWide": {
      "MinSpeedMagnitude": "0.35"
    }
  }
}
```

## How preset selection works

Each Mod instance in a synth config may optionally specify:

- `"preset": "PresetName"`

If no `preset` is present, the Mod uses `_default` implicitly.

## Precedence rules

For a given Mod instance with:

- `type = T`
- `presetKey = (preset if present else "_default")`

Values are applied in this order (later overrides earlier):

1) code defaults
2) `venue-presets.json`: `T["_default"]` then `T[presetKey]` if present
3) `mod-params/presets.json`: `T["_default"]` then `T[presetKey]` if present
4) per-config `mods.<ModName>.config`

Notes:

- Missing blocks are skipped (no validation required).
- This layering happens early (before default capture), so venue + preset defaults behave as true defaults.

## What belongs in each level

### Venue defaults (`venue-presets.json`)

Put values here when they are dependent on the physical venue or hardware and should apply across all configs:

- audio normalization ranges (min/max pitch/RMS/etc)
- audio detector thresholds/cooldowns
- video motion sampling + optical flow baseline
- motion magnitude baseline presets (when camera/lighting changes)

### Performance presets (`mod-params/presets.json`)

Put values here when the same parameter set appears across multiple configs:

- repeated `Fade` alpha values
- repeated `MultiplyAdd` maps
- repeated `Cluster` counts
- repeated “style blocks” for `SoftCircle` / `SandLine` / etc.

### Per-config overrides (`mods.*.config`)

Keep these for truly unique parameters:

- config-specific composition choices
- parameters that are intentionally different for that config’s role
- one-off experiments

## Worked example (before/after)

Goal: move repeated values into `mod-params/presets.json` and keep only the “unique bits” in the per-config file.

### Before

`performanceConfigRootPath/synth/06-minimal-video-ink.json` (example Mod excerpts):

```json
{
  "mods": {
    "FadeMarks": {
      "type": "Fade",
      "config": { "Alpha": "0.0012" }
    },
    "FadeMarksAlphaMap": {
      "type": "MultiplyAdd",
      "config": { "Multiplier": "0.0024", "Adder": "0.0012" }
    }
  }
}
```

### After

1) Put the repeated block(s) in `performanceConfigRootPath/mod-params/presets.json`:

```json
{
  "Fade": {
    "Alpha_0p0012": { "Alpha": "0.0012" }
  },
  "MultiplyAdd": {
    "Mul_0p0024_Add_0p0012": { "Multiplier": "0.0024", "Adder": "0.0012" }
  }
}
```

2) Update the synth config to reference presets and remove duplicated `config`:

```json
{
  "mods": {
    "FadeMarks": {
      "type": "Fade",
      "preset": "Alpha_0p0012"
    },
    "FadeMarksAlphaMap": {
      "type": "MultiplyAdd",
      "preset": "Mul_0p0024_Add_0p0012"
    }
  }
}
```

Result: the per-config file becomes more about composition/wiring, and less about re-stating shared tuning.

## Practical refactoring workflow

1) **Remove no-op constants first**
   - If a key is repeated everywhere and equal to the Mod’s own default, remove it.
   - Example: `AgencyFactor` when it is always `1` and not used for tuning.

2) **Remove deprecated/unused keys**
   - If the Mod no longer exposes a parameter, delete it from configs.
   - Example: `SomPalette.Iterations` (when iterations are fixed in code).

3) **Extract exact repeats into presets**
   - Cluster `mods.*.config` blocks by `type` and exact values.
   - Create a preset when you see 2+ identical blocks (or use a higher threshold for noisy types).
   - Apply by adding `preset` and deleting those keys from `config`.

4) **Extract near repeats cautiously**
   - Use numeric normalization so formatting differences don’t block matches (e.g. `"1"` vs `"1.0"`).
   - For small floating values, compare in log space (e.g. `MultiplyAdd` multipliers/adders) so `0.0001` and `0.05` are never treated as “close”.
   - Decide whether to:
     - keep differences as per-config overrides (safer), or
     - bake a representative value into a shared preset (more consolidation, slight behavior change).

5) **Move always-constant keys into `_default`**
   - If a key is present in many per-config blocks and never varies, it likely belongs in presets.

6) **Keep per-config files small**
   - After refactoring, each Mod’s `config` block should be short and “surprising” only when necessary.

## Identifying repeated parameter sets

Two useful notions:

- **Exact repeats**: same `type`, same `config` keyset, same values.
  - Treat numeric strings and numbers equivalently (`"1"` == `"1.0"`).
- **Near repeats**: same `type` + keyset, values “close enough”.
  - Prefer log-space bucketing for tiny scalars (typical for mapping coefficients).

Practical heuristics:

- Normalization for exact matching: parse floats where possible and compare with a tiny absolute epsilon.
- Near matching for small scalars:
  - compare `log10(abs(value))` within a bucket width (e.g. ±5–10% multiplicative), and keep sign separately
  - treat 0 as a special case (exact match only)

## Naming conventions

Presets should be named so a human can guess intent:

- Prefer semantic names when intent is obvious (e.g. `FadeUltraSlow`, `ImpulseSubtle`).
- If intent is ambiguous, numeric names are acceptable:
  - `Alpha_0p0012`
  - `Mul_0p0024_Add_0p0012`

A mixed approach is fine: semantic names for commonly understood roles, numeric names for dense parameter maps.

## Recommended invariants

- Per-config files should not include venue calibration knobs (keep those in `venue-presets.json`).
- Preset files are performance-scoped: do not store absolute device IDs or file paths there.
- Snapshot files are not a defaults mechanism.

## Related documents

- Implementation plan: `docs/venue-and-mod-presets-plan.md`
- Video tuning context: `docs/video-motion-tuning-plan.md`
- Audio tuning context: `docs/audio-tuning-inspector-plan.md`
- Future backlog: `docs/future-work.md`
