# Spectral Centroid Migration + SOM Palette Improvements Plan

## Context
The current audio→palette pipeline is sensitive to incoming range and can converge to low-variance / near-monochrome results. In addition, several exposed names (e.g. `ComplexSpectralDifferenceScalar`) are now misleading given the desired role of spectral centroid as an instrument-identifying feature.

This plan migrates the AudioDataSource outputs and configs from CSD-oriented naming to centroid-oriented naming, improves spectral point semantics so instruments "own" recognisable regions, makes `TimbreChange` a boolean pulse for reliable discrete events, and improves SOM palette variety while staying strictly audio/spectral-driven.

Notes:
- `spectralCentroid` from Gist is returned as a **bin index** (not Hz). Effective ranges depend on FFT/window size.
- We keep **aliases** during migration to avoid breaking configs mid-rollout.

## Goals
- Replace misleading CSD parameter/source names with centroid names.
- Keep onset detection based on CSD, but tune it via explicit detector scale parameters (not via scalar min/max).
- Make spectral point generators more instrument-identifying (centroid-driven).
- Make `TimbreChange` emit boolean pulses (`0.0` / `1.0`).
- Improve SOM palette variety without arbitrary color injection:
  - map learned features to RGB via a deterministic transform where centroid primarily affects brightness
  - select discrete palette chips by maximal separation

## Benchmark Configs
Use these performance configs to compare behaviour between phases:

### CaldewRiver
- `~/Documents/MarkSynth-performances/CaldewRiver/config/synth/31-movement4-meander-base.json`
- `~/Documents/MarkSynth-performances/CaldewRiver/config/synth/21-movement3-cataract-a.json`

### Improvisation1
- `~/Documents/MarkSynth-performances/Improvisation1/config/synth/14-palette-smear-audio-breath.json`
- `~/Documents/MarkSynth-performances/Improvisation1/config/synth/57-collage-video-particle.json` (good for `TimbreChange` event wiring)

If only testing two, start with:
- CaldewRiver: `31-movement4-meander-base.json`
- Improvisation1: `14-palette-smear-audio-breath.json`

## Phased Implementation Plan (Incremental)

### Phase 1 — Add centroid params/sources (keep aliases)
**Goal:** introduce correct names without breaking existing configs.

Code changes (ofxMarkSynth):
- `AudioDataSourceMod` adds:
  - Parameters: `MinSpectralCentroid`, `MaxSpectralCentroid`
  - Source: `SpectralCentroidScalar`
- Keep temporary aliases (one migration window):
  - Parameters: `MinComplexSpectralDifference`, `MaxComplexSpectralDifference` (alias to centroid min/max)
  - Source: `ComplexSpectralDifferenceScalar` (alias to centroid scalar)

Audio Inspector:
- Rename scalar row label from `CSD` to `Centroid`.
- Ensure the min/max names in the inspector match centroid.

Checkpoint:
- Existing performance configs load unchanged.

### Phase 2 — Redesign spectral point semantics (centroid-based)
**Goal:** make spectral points correspond more strongly to instrument identity.

Update `AudioDataSourceMod` point emissions:
- `Spectral3dPoint` = `{centroid, crest, ZCR}`
- `Spectral2dPoint` = `{centroid, crest}`
- `PolarSpectral2dPoint` = polar(angle=`centroid`, radius=`crest`)

Checkpoint:
- Benchmark configs run; composition changes expected and acceptable.

### Phase 3 — Onset tuning via detector scale params (Option 1)
**Goal:** keep CSD for onset detection but tune it properly.

Code changes (ofxAudioData):
- Add `OnsetSpectralScale` and `OnsetEnergyScale` parameters to `Processor::getParameterGroup()`.
- Replace hard-coded variance/scale constants in `Processor::detectOnset1()` with these parameters.

Checkpoint:
- Onset triggers still work with defaults; now venue-tunable.

### Phase 4 — Make `TimbreChange` a boolean pulse
**Goal:** "definite timbre change" causes "definite visible change".

Code changes (ofxAudioData):
- In `Processor::detectTimbreChange1()`:
  - keep z-score and cooldown logic
  - return `1.0f` when triggered, else `0.0f`

Checkpoint:
- `57-collage-video-particle.json` timbre-triggered events are reliable.

### Phase 5 — Extend Audio Inspector for event tuning (recommended)
**Goal:** venue calibration in one place.

Code changes (ofxMarkSynth):
- In Audio Inspector "Event detection" section:
  - make threshold/cooldown editable for Onset/Timbre/Pitch
  - add editable `OnsetSpectralScale` and `OnsetEnergyScale`
  - keep live z-score readouts

Checkpoint:
- Tune onset sensitivity while watching z-score.

### Phase 6 — Migrate performance configs to new centroid names
**Goal:** remove misleading legacy names from performance JSON.

Changes under `~/Documents/MarkSynth-performances/**`:
- In `*/config/synth/*.json`:
  - replace `Audio.ComplexSpectralDifferenceScalar` → `Audio.SpectralCentroidScalar`
- In `*/config/venue-presets.json`:
  - replace `MinComplexSpectralDifference` → `MinSpectralCentroid`
  - replace `MaxComplexSpectralDifference` → `MaxSpectralCentroid`

Checkpoint:
- All benchmark configs load with new names (aliases still present for safety).

### Phase 7 — Remove deprecated alias names
**Goal:** complete the cutover.

Code changes:
- Remove support for legacy centroid aliases:
  - `MinComplexSpectralDifference` / `MaxComplexSpectralDifference`
  - `ComplexSpectralDifferenceScalar` (as centroid alias)

Checkpoint:
- No legacy strings remain in repo or performance configs.

### Phase 8 — SOM palette variety improvements
**Goal:** avoid monochrome/flat outcomes while staying strictly spectral-driven.

#### 8A — Feature→RGB colorizer (centroid affects brightness)
Code changes (ofxSomPalette):
- Stop treating SOM features as literal RGB.
- Apply a deterministic 3×3 transform where:
  - centroid contributes equally to R/G/B (brightness axis)
  - crest/ZCR contribute to opponent chroma axes
- Add a `chromaGain` (and optionally `grayGain`) tuning control.

Checkpoint:
- SOM `FieldTexture` shows richer variation for the same audio.

#### 8B — Palette chip selection by maximal separation
Code changes (ofxSomPalette):
- Replace fixed edge sampling in `updatePalette()` with farthest-point sampling across the 16×16 field.

Checkpoint:
- `Random/Dark/Light` chips reflect the most distinct colors present.

### Phase 9 — Docs/examples + builds
- Update internal examples and docs:
  - `docs/synth-config-reference.json`
  - any repo example `*.json` using legacy names
- Build sanity:
  - `make -C example_audio`
  - `make -C example_audio_palette`

## Rollback / Safety Notes
- Keep Phase 1 aliases until Phase 6 configs are migrated and benchmarked.
- Avoid mixing Phase 6 config migrations with Phase 7 alias removals in the same iteration.
