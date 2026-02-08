# MarkSynth Performer Guide (MIDI-centric)

This guide is for **performers** (live control) rather than patch authors.

It is based on the MIDI mappings used by `apps/myApps/fingerprint2`.
Other host apps can map controls differently, but the concepts (Intent, Agency, layer alpha/pause, snapshots) are consistent.

Related:
- `docs/mods.md`: Mod reference (builder-facing)
- `docs/recipes.md`: wiring + tuning cookbook (builder-facing)

## Core concepts

### Synth Agency

`Synth Agency` is the global slider between manual control and autonomous behavior:
- Low agency: manual parameters dominate; graph auto/intent changes have less effect.
- High agency: connected/control-driven parameters take over more strongly.

Many Mods also expose an `AgencyFactor` that scales *their* effective agency (multiplies Synth Agency).

### Intent

Intent is a small set of high-level continuous controls (e.g. energy/structure/density/etc.).
In `fingerprint2` the MIDI controller exposes:
- 7 intent activation faders
- 1 master intent strength fader

If master strength is 0, individual intents effectively do nothing.

#### Intent → visuals (practical cheat sheet)

Exact intent mappings vary per Mod, but in the current `ofxMarkSynth` implementation the common patterns are:
- `Fade` / `Smear`: higher Density tends to shorten `HalfLifeSec` (faster clearing); Energy often increases “mix new” / activity.
- `SoftCircle`: Energy tends to increase `Radius`; Density tends to increase brightness/alpha.
- `Fluid`: Energy tends to increase `dt`; Chaos tends to increase vorticity (attenuated by Structure).
- `ParticleSet`: Density tends to increase `connectionRadius`; Energy increases simulation speed and force.
- `ParticleField`: Density tends to increase particle count; Energy/Chaos tend to increase field force/jitter.
- `SomPalette`: currently does not respond to Intent (manual params only).

Use this as a starting intuition; check `docs/mods.md` when a parameter feels “backwards”.

### Layer alpha + pause

- **Layer alpha** is a per-layer mix level in the final composite.
- **Layer pause** disables drawing into that layer (Mods should drop incoming events while paused).

In performance this gives you two different “mute” styles:
- lower alpha to fade a layer out
- pause to stop the layer being modified (often useful for freezing textures)

### Mod snapshots

Snapshots are performance recall of runtime parameter state (not the same as venue defaults or presets).
In `fingerprint2` you can load snapshot slots 1–8 from MIDI.

## Controller mappings (fingerprint2)

### Novation Launch Control XL 3 (DAW mode)

Reference: `apps/myApps/fingerprint2/docs/MIDI-Controller.md`.

#### Shift mode

Shift is a latching mode toggle.

| Mode | Top-row buttons | Bottom-row buttons | Faders | Play | Record |
|------|------------------|--------------------|--------|------|--------|
| Shift Off | Intent indicators | Load snapshots 1–8 | Intent activations | Pause/Play | Save image |
| Shift On  | Intent indicators | Load snapshots 1–8 | Layer alpha 1–8 | Hibernate | Save image |

Notes:
- Faders use **pickup mode** (soft takeover): move past the current value before it takes effect.
- Top-row buttons are **LED indicators only**.

#### Transport buttons

| Button | Shift Off | Shift On |
|--------|-----------|----------|
| Shift | Toggle shift mode | Toggle shift mode |
| Play | Pause/Play | Hibernate |
| Record | Save image | Save image |
| Track Left | Previous config | Previous config |
| Track Right | Next config | Next config |

#### Encoders (knobs)

These are tuned for quick **audio calibration** in a venue.
They map onto `AudioDataSourceMod` parameters (typically better stored in `venue-presets.json`, but exposed here for live correction).

Row 1:
- Encoder 1: Synth Agency
- Encoder 3: `Audio.MinPitch`
- Encoder 4: `Audio.MaxPitch`
- Encoder 5: `Audio.MinComplexSpectralDifference`
- Encoder 6: `Audio.MaxComplexSpectralDifference`

Row 2:
- Encoder 11: `Audio.MinRms`
- Encoder 12: `Audio.MaxRms`
- Encoder 13: `Audio.MinSpectralCrest`
- Encoder 14: `Audio.MaxSpectralCrest`
- Encoder 15: `Audio.MinZeroCrossingRate`
- Encoder 16: `Audio.MaxZeroCrossingRate`

Practical calibration tips:
- If quiet passages produce no marks (everything “dead”): lower `MinRms`.
- If everything is always loud (no dynamics): raise `MaxRms` and/or raise `MinRms` slightly.
- If pitch-driven visuals sit at a constant value: widen `MinPitch`/`MaxPitch`.

#### Intent indicator LEDs (top row)

These indicate whether the corresponding intent parameter exists and whether it is active.
- Bright yellow: value > 0 and master strength > 0
- Dim yellow: parameter exists but is 0 (or master strength is 0)
- Off: no matching intent parameter exists

#### Snapshot buttons (bottom row)

Buttons 9–16 load snapshot slots 1–8.

### Akai APC Mini MK2

`fingerprint2` also supports the APC Mini MK2 as a second controller.
It is mainly used for fast **config selection** and **layer toggles**.

#### Config grid (8×7)

- Pads in rows 1–7 select configs in the Performance grid.
- Selection is **hold-to-confirm** (to avoid accidental switches).
- Non-current configs are shown dim; current config is shown at full brightness.

#### Layer pause toggles

- The bottom row of pads (row 0, notes 0–7) toggles pause for layers 1–8.
- The physical bottom buttons (notes 100–107) can also act as layer toggles and status indicators (red-only LEDs).

#### Faders

- Faders 1–8: layer alpha 1–8 (soft takeover; ~5% pickup)
- Master fader: Synth Agency

## Mod-specific live control notes

This section is performer-facing: safe, high-leverage knobs and common failure modes.
For full parameter/sink details see `docs/mods.md`.

### DividedArea (geometric structure)

What it feels like live:
- Major lines = large-scale architectural structure.
- Minor lines = detail texture / subdivision.

Good live knobs:
- `Strategy` + `Angle`: changes how minor lines are generated.
- `unconstrainedSmoothness`: how “calm” major lines feel.
- `MajorLineWidth` / `PathWidth`: thickness of major vs minor geometry.
- `MajorLineColour` / `MinorLineColour`: contrast and readability.

Live tactics:
- If the picture is too busy: reduce minor input density (or pause the default/minor layer) and keep major lines.
- If major lines feel twitchy: raise `unconstrainedSmoothness` and/or increase Structure intent.
- To freeze structure: pause the `major-lines` layer (or lower its alpha to fade it out).

Gotchas:
- `ChangeStrategy` has a built-in cooldown (~5s), so it won’t rapid-cycle.
- Refraction/chromatic major line styles only work when the `major-lines` drawing layer is an overlay.

### Collage (path + snapshot)

What it feels like live:
- Collage is event-driven: it only draws when it has both a valid `Path` and (for snapshot modes) a fresh `SnapshotTexture`.

Good live knobs:
- `BlendMode` + `Opacity`: biggest impact on “paper vs light” feel.
- `Saturation`: quickly moves between washed and vivid.
- `OutlineAlphaFactor` / `OutlineWidth` / `OutlineColour`: outline readability and emphasis.
- `MinDrawInterval`: throttle when paths arrive too quickly.

Live tactics:
- If everything looks blown out: reduce `Opacity` first; then consider switching `BlendMode` to ALPHA.
- If nothing appears:
  - switch `Strategy` to `0` (tint fill) to verify paths are arriving
  - check whether the outlines layer is paused/transparent (outlines can help confirm activity)

Gotchas:
- Snapshot strategies require a stencil-enabled drawing layer; if that’s missing you’ll see errors and no draw.

### Fluid (motion field + dye)

What it feels like live:
- A slow-moving “atmosphere” generator.
- Also the main source of velocity textures for downstream Mods (`ParticleField`, `Smear`).

Good live knobs:
- `dt`: overall simulation speed (higher = faster motion, but less stable).
- `Velocity Dissipation`: how long motion persists.
- `Value Dissipation`: how long dye/brightness persists.
- `Vorticity`: turbulence / curl strength.
- `Boundary Mode`: only change when you *mean* it (wrap vs walls changes the whole feel).
- Velocity-field injection: `VelocityFieldMultiplier` (fine) and `VelocityFieldPreScaleExp` (coarse).

Live tactics:
- If the sim looks “dead”: increase `dt` a little and/or increase `VelocityFieldMultiplier` (if injecting an external field).
- If everything smears into mush: increase `Value Dissipation` (more dye persistence) but decrease `Value Spread` (less diffusion).
- If it blows out / becomes noisy: reduce `dt` first, then increase dissipation.

Gotchas:
- If you’re injecting an external velocity field, its effective scale is `10^VelocityFieldPreScaleExp * VelocityFieldMultiplier`; small exponent changes are huge.

### Smear (feedback displacement)

What it feels like live:
- A feedback processor: it drags the previous frame through a vector field.

Good live knobs:
- `MixNew`: overall feedback amount (lower = stronger smear/feedback, higher = cleaner).
- `HalfLifeSec`: trail persistence (higher = longer memory).
- `Field1Multiplier` / `Field2Multiplier`: field strength.
- `Field1PreScaleExp` / `Field2PreScaleExp`: coarse field normalization (log10).
- `Strategy`: spatial teleport/fold modes (use sparingly live).

Live tactics:
- If the image collapses into a fog: raise `MixNew` and/or lower `HalfLifeSec`.
- If smear does nothing: increase `Field1Multiplier` first; if still weak, increase `Field1PreScaleExp` by +1.
- If it tears violently: reduce `Field*Multiplier` and/or reduce `Field*PreScaleExp`.

Gotchas:
- Smear is sensitive to persistence: very long `HalfLifeSec` + strong fields can accumulate into blowout quickly.
- If a patch uses strategy modes, switching `Strategy` mid-performance can change the entire topology (great when deliberate; risky otherwise).

### Quick triage table

First knobs to try when something suddenly feels wrong:

| Symptom | First knob(s) | Next knob(s) |
|--------|--------------|--------------|
| Nothing draws / feels dead | Increase layer alpha; unpause layer | Lower `Audio.MinRms`; check `Synth Agency` isn’t 0 |
| Everything is blown out / too bright | Lower layer alpha; reduce `Collage.Opacity` | Switch `Collage.BlendMode` to ALPHA; reduce `SoftCircle.ColourMultiplier` |
| Trails too long / image won’t clear | Lower `Fade.HalfLifeSec` / `Smear.HalfLifeSec` | Increase `Smear.MixNew`; consider pausing a feedback-heavy layer |
| Image turns to fog / mush | Increase `Smear.MixNew` | Lower `Smear.Field*Multiplier` / `Fluid.dt` |
| Smear has no effect | Increase `Smear.Field1Multiplier` | Increase `Smear.Field1PreScaleExp` by +1 (big step) |
| Fluid feels frozen | Increase `Fluid.dt` slightly | Increase `Fluid.VelocityFieldMultiplier` (if injecting a field) |
| Fluid is unstable / noisy | Reduce `Fluid.dt` | Increase `Fluid.Velocity Dissipation`; reduce impulse strength upstream |
| Geometry too busy | Lower minor layer alpha or pause minor layer | Reduce point density upstream; increase Structure intent |
| Colors feel washed out | Increase `Collage.Saturation` | Drive `Colour` from a palette source; increase `SoftCircle.ColourMultiplier` carefully |

## Suggested live workflow

1) **Load a config** (APC grid or Track Left/Right).
2) **Set agency** low to start (predictable manual state), then increase as you want autonomy.
3) **Calibrate audio** quickly if needed (RMS + pitch min/max encoders).
4) **Bring in Intent**: raise master strength, then add individual intent activations.
5) **Mix layers**: use layer alpha to balance composition; pause layers to freeze.
6) **Recall snapshots** to jump between tuned states.
7) **Hibernate** to freeze the system (and unhibernate to resume).

## Troubleshooting

- Nothing responds: check MIDI device is connected and in the expected mode (Launch Control XL 3 must be in DAW mode for LED/display).
- Faders feel "stuck": pickup mode is active; move past the current value.
- Audio feels over/under-sensitive: adjust `MinRms`/`MaxRms` first; then spectral crest ranges.
