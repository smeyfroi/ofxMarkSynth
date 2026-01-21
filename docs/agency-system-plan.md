# Agency System (with Intent) — Context + Step-by-Step Plan

This document captures the agreed design for the **Agency system** and a detailed implementation plan.

Agency works *in tandem* with Intent:
- **Intent** describes artistic direction (E/D/S/C/G) and drives deterministic mappings.
- **Agency** determines *how much performer-driven signals (audio/video) can override / steer* parameters and events.

The immediate goal is to enrich Agency so physical performers can reliably create:
- **Subtle continuous influence** (colors, alpha, motion amounts) via existing ParamController auto-mixing.
- **Discrete structural changes** (layer/register shifts and strategy switches) gated by performer activity.

We start with **CaldewRiver** as the proving ground.

---

## Current State (as implemented)

### Agency in ParamController
`ParamController<T>` blends three contributions:
- **Auto** (external connections / generated values)
- **Intent** (intent mappings)
- **Manual** (GUI-set / config-set)

Agency affects the **outer weighting** between auto vs human:
- `effectiveAgency = hasReceivedAutoValue ? agency : 0.0f`
- `wAuto = effectiveAgency`
- `wManual` and `wIntent` share the remainder, modulated by manual bias.

See: `src/core/ParamController.h`

### Synth Agency parameter
- Config `synth.agency` currently sets `Synth Agency` (manual baseline), parsed in `src/config/SynthConfigSerializer.cpp`.
- Mods frequently scale it using an `AgencyFactor` parameter (e.g. `getAgency() = Mod::getAgency() * AgencyFactor`).

See: `src/core/Synth.hpp`, `src/core/Mod.*`

### Existing event-style sinks
Multiple Mods already support discrete sinks that can serve as “Agency-triggered” events:
- `ChangeLayer` (various Mods)
- `DividedArea.ChangeStrategy`

However, in CaldewRiver many Mods target a single drawing layer, so `ChangeLayer` often has no effect.

---

## Design Goals + Constraints (agreed)

### Goals
1. **Performer-driven Agency should be the primary bias**, sourced from audio + video.
2. **No calibration to a ‘mid’ value**.
3. **Stimulus should be symmetric**: quiet→loud and loud→quiet both count.
4. **Patch wiring defines policy**:
   - multiple Agency controllers per patch
   - each controller can target different sinks (layer change vs strategy change)
5. **Begin with three discrete ‘areas’ (CaldewRiver)**:
   - Smear register shifting (`Smear.ChangeLayer`)
   - ParticleField register shifting (`ParticleField.ChangeLayer`)
   - DividedArea strategy shifting (`DividedArea.ChangeStrategy`)
6. **Keep MemoryEmit and TextSource out for now** (can be added later).

### Constraints
- Treat incoming stimulus as one combined stream (no audio/video split agency lanes for now).
- Accept a one-frame delay; **do not add Synth update ordering complexity**.
- Trigger semantics should be consistent: **event triggers when value > 0.5**.
- Budget/Stimulus do **not** need to be formal sources; GUI can access ivars (via getters).
- Node editor is visible during performance; performer-facing UI should be minimally cluttered.

---

## Proposed Architecture

### New Process Mod: `AgencyControllerMod`
A small reusable Mod that:
- consumes a smoothed characteristic
- derives stimulus = abs(delta(smoothed characteristic))
- integrates a budget
- produces:
  - a continuous `AutoAgency` value
  - a discrete `Trigger` pulse

Multiple instances may exist in the same config.

#### Sinks (inputs)
- `Characteristic` (float)
  - may receive multiple connections; the mod uses per-frame `max()` to combine
- `Pulse` (float)
  - may receive multiple pulse inputs; pulse is considered “active” when `value > PulseThreshold` (default 0.5)

#### Sources (outputs)
- `AutoAgency` (float 0..1)
- `Trigger` (float 0 or 1)

#### Parameters (initial defaults, tune later)
- `CharacteristicSmoothSec`
- `StimulusSmoothSec`
- `ChargeGain`
- `DecayPerSec`
- `AutoAgencyScale`
- `AutoAgencyGamma`
- `PulseThreshold` (default 0.5)
- `EventCost`
- `CooldownSec`

#### Internal state (ivars; exposed via getters)
- `characteristicSmooth`, `characteristicPrev`
- `stimulusRaw`, `stimulusSmooth`
- `budget`
- `autoAgency`
- `lastTriggerTimeSec`

### Stimulus model (no ‘mid’ calibration)
Per controller, per frame:
1. `characteristicRaw = max(receivedCharacteristicValuesThisFrame)`
2. Smooth: `characteristicSmooth = smoothToFloat(characteristicSmooth, characteristicRaw, dt, CharacteristicSmoothSec)`
3. `stimulus = abs(characteristicSmooth - characteristicPrev)`
4. Budget: `budget += ChargeGain * stimulus; budget -= DecayPerSec * dt; clamp to [0,1]`
5. `autoAgency = clamp(AutoAgencyScale * pow(budget, AutoAgencyGamma), 0..1)`

This makes both rising and falling performer activity contribute.

### Synth integration: `.AgencyAuto` sink
Add a Synth sink `AgencyAuto` that accepts 0..1 values from AgencyControllerMod(s).
- Multiple controllers may connect; Synth aggregates with `max()`.
- Synth computes effective agency:
  - `effectiveAgency = clamp(manualAgency + autoAgencyAggregate, 0..1)`

No special ordering logic is added; we accept potential frame-latency.

---

## GUI Plan (agreed)

### Synth window: aggregate indicator only
Near the existing `Synth Agency` slider:
- Show only aggregate: Manual + Auto = Effective
- Use a minimal bar/label (no list of controllers).

Example representation:
- `Manual 0.25 + Auto 0.18 = 0.43`
- Optional thin bar with two colored segments (manual vs auto).

### Node editor: live controller state on nodes (author/operator-facing)
Since the node editor is visible:
- On `AgencyControllerMod` nodes, show a small titlebar progress bar representing either:
  - `budget` (recommended), or
  - `autoAgency`
- Provide a hover tooltip for AgencyControllerMod nodes showing:
  - `CharacteristicSmooth`
  - `Stimulus`
  - `Budget`
  - `AutoAgency`

This mirrors existing node titlebar indicators already used for agency-active nodes.

---

## Trigger Semantics (agreed)

To reduce confusion, triggers should use the same rule everywhere:
- **Trigger happens when sink value > 0.5**

This implies we should simplify any sink logic that currently uses multi-band thresholds.

### Target Mods for consistent triggers
- `SmearMod` (`ChangeLayer` currently has multiple thresholds)
- `ParticleFieldMod` (`ChangeLayer` uses two thresholds)
- `DividedAreaMod` (`ChangeLayer` threshold is 0.6)

Plan: change these to a single behavior:
- `if (value > 0.5f) changeDrawingLayer();`

(If we later want reset/disable semantics, we will introduce separate sink names rather than threshold bands.)

---

## CaldewRiver Wiring Plan (first iteration)

We start with **multiple controllers per patch**, one per target domain:

1) `AgencySmear`
- `Trigger` wired to `Smear.ChangeLayer`

2) `AgencyParticles`
- `Trigger` wired to `ParticleField.ChangeLayer`

3) `AgencyDividedArea`
- `Trigger` wired to `DividedArea.ChangeStrategy`

All three controllers can share the same combined characteristic and pulse inputs initially.

### Characteristic inputs (combined with max)
- Audio-only patch: `Audio.RmsScalar -> Agency*.Characteristic`
- Video-only patch: `MotionMagnitude.MeanScalar -> Agency*.Characteristic`
- AV patch: connect both; max combine happens inside AgencyControllerMod.

### Pulse inputs (all enabled)
- `Audio.Onset1 -> Agency*.Pulse`
- `Audio.TimbreChange -> Agency*.Pulse`
- `Audio.PitchChange -> Agency*.Pulse`
- Motion spikes: `MotionMagnitude.MaxScalar -> Agency*.Pulse`

### Make `ChangeLayer` effective
`ChangeLayer` only works if the target Mod has multiple drawing layers assigned for its layer key.

Config changes required:
- Add additional `drawingLayers` (e.g. `smear_a`, `smear_b`, `smear_c`)
- Assign multiple layers to the Mod’s layer slot:
  - `mods.Smear.layers.default = ["smear_a", "smear_b", "smear_c"]`

See parsing: `src/config/SynthConfigSerializer.cpp` (layers arrays are supported).

---

## Step-by-Step Implementation Plan

### Phase 0 — Documentation and scope lock
- [x] Create this document.
- [ ] Confirm the three initial targets in CaldewRiver are: Smear, ParticleField, DividedArea.

### Phase 1 — Implement `AgencyControllerMod`
1. Add new files:
   - `src/processMods/AgencyControllerMod.hpp`
   - `src/processMods/AgencyControllerMod.cpp`
2. Add to `src/ofxMarkSynth.h` includes.
3. Register in `src/config/ModFactory.cpp` as type `"AgencyController"` (or `"AgencyControllerMod"` if preferred in JSON; keep type name short and consistent).
4. Add getters for GUI: `getBudget()`, `getStimulus()`, `getAutoAgency()`, etc.

### Phase 2 — Add Synth sink `.AgencyAuto`
1. Add sink ID and mapping in `src/core/Synth.hpp` / `src/core/Synth.cpp`.
2. Store `autoAgencyAggregate` (max of received values).
3. Reset aggregate once per frame.
4. Update `Synth::getAgency()` to return `clamp(agencyParameter + autoAgencyAggregate, 0..1)`.

### Phase 3 — GUI (aggregate + node tooltips)
1. In `src/core/Gui.cpp`:
   - Add aggregate indicator near `Synth Agency` slider.
2. In node editor rendering (`Gui::drawNode`):
   - Detect `AgencyControllerMod` nodes
   - Render a small progress bar in title bar (budget)
   - Add tooltip with live values.

### Phase 4 — Normalize trigger semantics (>0.5)
Update the targeted Mods:
- `src/layerMods/SmearMod.cpp`
- `src/sinkMods/ParticleFieldMod.cpp`
- `src/sinkMods/DividedAreaMod.cpp`

Make `ChangeLayer` (and DividedArea ChangeStrategy trigger behavior if needed) respond to `> 0.5f` consistently.

### Phase 5 — CaldewRiver patch wiring (initial)
For one representative config from each “area”:
1. Add drawing layers (multi-layer targets).
2. Add `AgencyControllerMod` instances:
   - `AgencySmear`, `AgencyParticles`, `AgencyDividedArea`
3. Wire:
   - `Agency*.AutoAgency -> .AgencyAuto`
   - `AgencySmear.Trigger -> Smear.ChangeLayer`
   - `AgencyParticles.Trigger -> ParticleField.ChangeLayer`
   - `AgencyDividedArea.Trigger -> DividedArea.ChangeStrategy`
4. Connect characteristic + pulse sources.

### Phase 6 — Tuning + iteration
- Tune decay/charge/cost/cooldown values per controller.
- Decide whether different controllers should receive different subsets of pulses (policy remains patch wiring).

---

## Notes / Future Extensions (explicitly deferred)
- Memory bank event gating (`MemoryEmit*`) — deferred.
- TextSource event gating (`TextSource.Next`) — deferred.
- Separate audio-vs-video agency lanes — deferred.
- Re-introducing Smear reset/disable — defer until we have explicit sink names rather than threshold bands.
