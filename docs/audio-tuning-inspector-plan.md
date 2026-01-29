# Audio Tuning / Inspection Plan (Venue Calibration)

## Context

MarkSynth is driven by a single Synth-owned audio analysis client (`ofxAudioAnalysisClient::LocalGistClient`) and a per-`AudioDataSourceMod` `ofxAudioData::Processor`.

We can rehearse against pre-recorded material, but venue acoustics / mic gain / PA dynamics can invalidate the tuned ranges used to normalise analysis scalars.

This plan defines a *performance-focused* workflow and UI for fast venue calibration without editing every performance config.

---

## Current Behaviour (Important)

### Scalars used by `AudioDataSourceMod`

`AudioDataSourceMod` emits these scalars (all normalised):

- `pitch`
- `rootMeanSquare` (RMS)
- `complexSpectralDifference` (CSD)
- `spectralCrest`
- `zeroCrossingRate` (ZCR)

See `src/sourceMods/AudioDataSourceMod.hpp` and `src/sourceMods/AudioDataSourceMod.cpp`.

### Wrapped normalisation (no clamping)

Normalisation is currently **wrapped**, not clamped:

- `u = ofMap(raw, min, max, 0..1)` (unclamped)
- `w = frac(abs(u))`

So values outside the configured range **alias / wrap back into [0..1)**.

This is implemented in `ofxAudioData::Processor::normalisedValue()`.

### Filter usage

For the normalised values used by `AudioDataSourceMod`, the code path currently uses `Processor::getNormalisedScalarValue(..., filterIndex=1, isLinear=true)`.

This means the main outputs are based on the **smoother filtered values** (filter set 1), not raw.

Event detection (`Onset1`, `TimbreChange`, `PitchChange`) uses a mix of raw + smoothed baseline inside `ofxAudioData::Processor`.

---

## Goals

- Provide an **ImGui-based tuning mode** to calibrate venue audio quickly.
- Avoid proliferating windows: the tuning UI lives inside the existing ImGui **Debug View**.
- Keep DearImGui dependencies **out of** `ofxAudioAnalysisClient` and `ofxAudioData`.
- Keep wrapped normalisation (avoid hotspots at extremes).
- Initially keep the tool simple; add “30s capture” calibration later.

---

## UI / Interaction Plan

### Keybindings

- `t` (lowercase): toggle current live visualisation overlay (existing tuning overlay evolves into a lightweight view).
- `T` (uppercase): toggle **Tuning Mode** (new Audio Inspector UI).

### Debug View integration

The existing ImGui window `Debug View` currently shows the debug FBO texture.

Plan: add a simple mode switch inside that window:

- **View: FBO** (current behavior)
- **View: Audio Inspector** (new)

Implementation options (choose when implementing):
- `ImGui::BeginTabBar` (preferred)
- or a compact radio toggle.

---

## Audio Inspector (Phase 1: “Live Stats”, no capture)

This is a **venue-calibration** tool, not a general DAW.

For each scalar (Pitch, RMS, CSD, SpectralCrest, ZCR), show:

- Current filtered value (matching filter index used by the synth)
- Configured `min` / `max` values (from the active `AudioDataSourceMod`)
- Unwrapped normalised value `u` (can be <0 or >1)
- Wrapped normalised value `w` (what the synth uses)
- Out-of-range statistics over a rolling window (e.g. 10–30 seconds):
  - `% (u < 0)`
  - `% (u > 1)`
  - optional median/95% overshoot magnitude

For event detectors (Onset/Timbre/PitchChange), show:

- current z-score
- threshold value
- cooldown remaining

(Events/minute and capture-based tuning are deferred to Phase 2.)

---

## Architecture Plan (No ImGui in audio addons)

### Model (no ImGui)

Add an `AudioInspectorModel` (name TBD) within `ofxMarkSynth` (not in audio addons) that:

- reads scalar values from the Synth-owned audio client / processor
- maintains small rolling buffers
- computes `u`, `w`, and out-of-range stats
- exposes plain C++ structs for the UI

### View (ImGui)

Add an ImGui renderer inside `ofxMarkSynth::Gui` that:

- draws the Audio Inspector view inside the existing Debug View
- calls model update methods

This keeps `ofxAudioAnalysisClient` and `ofxAudioData` free of any DearImGui dependency.

---

## Phase 2 (Future): 30s Capture + Suggestions

Add a “Capture” button (e.g. 30 seconds) that computes percentiles per scalar (`p05/p50/p95`) to propose effective tuning.

Potential outputs:

- suggested effective min/max per scalar (real units)
- suggested multiplier deltas relative to the currently-loaded config

---

## Related Work (Next after Inspector)

- Add Synth-level “venue tuning” parameters (persistent across config switches) that apply as multipliers/offsets on top of per-config tuning ranges.
- Rebind MIDI encoders (Launch Control XL 3) to these Synth-level levers for fast venue calibration.
