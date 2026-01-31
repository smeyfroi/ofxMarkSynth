
# Venue + Mod Presets Unification Plan

Goal: unify venue calibration defaults and type/preset defaults into a single, generic mechanism that:

- applies before `Mod::defaultParameterValues` are captured (so defaults don’t get re-saved into synth configs)
- works for all Mod types (no special-casing audio/video)
- replaces and deletes the old tuning override systems (`AudioTuningOverrides`, `VideoMotionOverrides`)

This document is the shared implementation plan referenced by:

- `docs/video-motion-tuning-plan.md`
- `docs/audio-tuning-inspector-plan.md`

## Scope

- Performance scope only (shared across all configs in a performance folder)
- No backwards compatibility

## Files (performance scope)

- Venue defaults: `performanceConfigRootPath/venue-presets.json`
- Type/preset defaults: `performanceConfigRootPath/mod-params/presets.json`
- Snapshots remain unchanged:
  - `performanceConfigRootPath/mod-params/snapshots/<configId>.json`

## JSON schema (same for both preset files)

Each preset file is a JSON object keyed by Mod `type` (factory type string, e.g. `VideoFlowSource`, `AudioDataSource`, `VectorMagnitude`).

Within each `type`, keys are:

- `_default`: defaults for all mods of this type
- `<PresetName>`: defaults for mods of this type that specify `"preset": "<PresetName>"` in the synth config

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
  },
  "VectorMagnitude": {
    "_default": {
      "DecayWhenNoInput": "0.95"
    },
    "MotionMagnitudeAgency": {
      "Components": "zw",
      "Max": "0.02"
    }
  }
}
```

## Precedence (per Mod instance)

Let a Mod instance have:

- `type = T`
- `presetKey = (preset if present else "_default")`

Defaults are applied in this order (later overrides earlier), before per-mod `config` is applied:

1) code defaults (`ofParameter` constructor values)
2) `venue-presets.json`:
   - `T["_default"]`
   - `T[presetKey]` (if present)
3) `mod-params/presets.json`:
   - `T["_default"]`
   - `T[presetKey]` (if present)
4) synth config `mods[modName].config` values (per-instance overrides)

Notes:

- If a block is missing, it is skipped (no validation required).
- If a Mod has no `preset` key, it is `_default` by definition.

## Implementation Plan (build order)

1) Replace `ModPresetLibrary` with a generic preset loader for the new schema.
   - Inputs: file path + (`type`, `presetKey`)
   - Output: flattened `ModConfig` map (paramName -> valueString)
   - Cache: by file path + mtime

2) During config load (`SynthConfigSerializer`):
   - Read `type` and optional `preset` for each Mod config entry
   - Compute `presetKey`
   - Load defaults from `venue-presets.json` and `mod-params/presets.json`
   - Merge defaults according to precedence rules
   - Call `modPtr->setPresetConfig(mergedDefaults)`

3) Delete legacy override systems (no compatibility layer):
   - remove `src/config/AudioTuningOverrides.*`
   - remove `src/config/VideoMotionOverrides.*`
   - remove related methods from `src/core/Synth.*`
   - remove related UI controls in `src/core/Gui.*`

4) Simplify config saving (`Synth::updateModConfigJson`):
   - remove special-case stripping of “tuning keys”
   - rely on the captured defaults (which now include venue + preset defaults)

5) Migrate performance folders:
   - convert any existing tuning override values into the new preset files
   - stop using `performanceConfigRootPath/tuning/*.json`

## Future work

See `docs/future-work.md` (Venue + Mod Presets Unification).
