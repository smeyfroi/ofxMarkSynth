# Intent-Only Config Analysis (Performer-Facing)

This document describes a repeatable, code-driven way to summarize what a single Synth config will look like under **Intent-only control**.

It is written for performer prep: the output is a short description of what each *intent preset* (e.g. Still / Sparse / Flow / …) tends to do visually in that config, and which Intent dimensions are the strongest levers.

See also:
- `docs/intent-mappings.md` (per-Mod mapping reference)
- `docs/intent-implementation.md` (how Intent is implemented in code)

---

## Goal

Given a config JSON, produce:

1. A list of the **major visible subsystems** in that patch (e.g. smear layer, particle webs, fluid impulses, divided-area geometry).
2. For each intent preset (Still/Sparse/…): a concise description of the **expected visual state**.
3. A statement of which Intent dimensions are the **primary performance levers** for that patch.

---

## The "Intent-only" mental model

### Why Intent-only is useful

In the full synth, each parameter can be influenced by:

- **Auto** (connections)
- **Manual** (live UI edits)
- **Intent** (the active intent preset)

Intent-only analysis tries to isolate the `applyIntent()` behavior so you can build intuition about what the intent presets *want* to do in a patch, before adding back agency/auto/manual interactions.

### How the engine blends Auto/Manual/Intent

Blending and weighting happens in `src/core/ParamController.h`.

Key points from `src/core/ParamController.h:251`:

- **Auto share** is controlled by *agency*, but only for parameters that actually receive auto values.
- Intent is part of the remaining **"human share"**.
- Intent has no effect unless the Mod actually calls `updateIntent()` for that parameter.

Important nuance:
- There is a global baseline manual share (`ParamControllerSettings::baseManualBias`), so even with Intent Strength 1.0, intent is usually "almost all" rather than mathematically 100%.

For performer intuition, the following approximation is usually good:

- Set `synth.agency = 0.0`
- Set a single intent activation to `1.0`
- Set `Intent Strength = 1.0`
- Avoid manual edits (so manual bias stays low)

This yields the most “pure” intent behavior without changing engine settings.

---

## Repeatable analysis method

### Step 1: Read the config and list visible layers/mods

In the config JSON (from a performance set like Improvisation1, or any other set you’re analyzing):

- Identify drawing layers and which are drawn (`drawingLayers.*.isDrawn`).
- Identify “headline” Mods (sinks/layerMods): `Smear`, `Fluid`, `ParticleSet`, `ParticleField`, `DividedArea`, `Collage`, `SandLine`, etc.

For the worked example (config 60 from the Improvisation1 set), those are in `config/synth/60-chaos-av-dualparticle.json:85` and `config/synth/60-chaos-av-dualparticle.json:180`.

### Step 2: Identify what will still be active when `agency=0`

With `agency=0`:

- **Streams still flow**: point sources and textures still propagate through connections.
- **Auto parameter controls are suppressed**: any `receive()` calls that do `controller.updateAuto(..., getAgency())` will have weight 0.

This matters because many visual characteristics (especially palette-driven color) are auto-wired.

Practical outcome:
- Under intent-only conditions, mods that do their own intent color composition will still shift color.
- Mods that only receive color from connections will tend to stick to their manual/default color.

### Step 3: For each major Mod in the config, read its `applyIntent()`

For each Mod type present, locate its implementation and read `applyIntent()`.

Example paths used for config 60:

- Video sampling density: `src/sourceMods/VideoFlowSourceMod.cpp:112`
- Smear behavior: `src/layerMods/SmearMod.cpp:199`
- Fluid impulses: `src/sinkMods/FluidRadialImpulseMod.cpp:95`
- Particle webs: `src/sinkMods/ParticleSetMod.cpp:146`
- Divided-area geometry: `src/sinkMods/DividedAreaMod.cpp:245`

This step is the core: **we trust code, not assumptions**.

### Step 4: Translate parameter changes into visual outcomes

This is the “artist translation” step.

Take each mapped parameter and rephrase it visually:

- If intent drives `Colour.alpha` (or a color controller’s `.a`), that is a **presence/opacity** lever.
- If intent drives `PointSamplesPerUpdate`, that is an **injection rate / density of activity** lever.
- If intent drives `Impulse Strength` / `Impulse Radius`, that is a **motion-field energy/scale** lever.
- If intent drives smear `gridSize`, `gridLevels`, `jumpAmount`, that is a **order-vs-chaos texture** lever.

### Step 5: Apply the intent preset values (E/D/S/C/G)

For each intent preset in the config’s `intents` list, plug the E/D/S/C/G values into the mapping logic from Step 3.

For config 60, presets live at `config/synth/60-chaos-av-dualparticle.json:27`.

You do not need to calculate exact numeric results to get a performer-useful summary; the important thing is the *direction*:

- Low vs high
- Which dimensions spike together (e.g. Turbulence has high E and high C)

### Step 6: Determine “primary levers”

For the given config:

- A dimension is a **primary lever** if it affects parameters that strongly change visibility, density, motion speed, or large-scale structure.
- A dimension is **secondary** if it mainly refines texture/detail.

In practice, primary levers are usually:

- `D` (opacity / density / injection)
- `E` (speed / intensity)
- `C` (turbulence / randomness)

…but this is patch-dependent.

---

## Delta-Based “Impact” Tables (Recommended)

Per-intent descriptions ("Still feels calm", etc.) are useful, but for live performance prep it’s often more actionable to ask:

- **When I switch to intent X, which mark types change most?**

This is a **delta** analysis: you choose a baseline state, then rate how much each mark type changes when switching into each intent.

### Choose a baseline

Two baselines are especially useful:

- **Baseline = Still**: answers “how far from my calm register is this intent?”
- **Baseline = No intent**: answers “how far from the patch’s manual/default behavior does this intent pull?”

For “No intent”, set either:

- All intent activations to 0 (so total activation = 0), or
- `Intent Strength = 0`

With `agency=0`, “No intent” is approximately “manual config values” (auto-wired parameter changes are suppressed).

### Mark-type intensity proxies (+ / -)

To add a direction hint, choose a simple per-mark **intensity/presence proxy**, and compare intent vs baseline.

This is intentionally heuristic: it should be *approximately meaningful*, not mathematically perfect.

Example proxy interpretations:

- **Smear**: `+` = more active/stronger smear (more refresh, stronger field distortion), `-` = calmer/weaker smear
- **Webs** (ParticleSet/ParticleField): `+` = more present/opaque, `-` = less present
- **Geometry** (DividedArea): `+` = more visible linework / more dominant geometry register, `-` = less visible
- **Soft marks** (SoftCircle): `+` = larger/more present circles, `-` = smaller/more latent
- **Fluid**: `+` = more motion energy (stronger/larger impulses, faster evolution), `-` = calmer

### Compact notation for live reference

When writing delta tables, annotate each cell using:

- `++` = high change, increase
- `+`  = medium change, increase
- `-`  = medium change, reduction
- `--` = high change, reduction
- *(blank)* = little effect (low delta)

### Table layout (easy to scan live)

For performer-facing summaries, prefer:

- **Intents as columns**, **mark types as rows**.
- Keep the same intent column order in every table: `Still`, `Sparse`, `Flow`, `Grain`, `Structure`, `Turbulence`, `Maximum`.
- Order mark rows in the order you visually perceive them (a “scene stack”), e.g.:
  1. **Surface** (Smear / Fluid wash)
  2. **Motion driver** (FluidMotion)
  3. **Particles** (Webs / ParticleField)
  4. **Accents** (SoftMarks)
  5. **Geometry overlay** (Geometry)

Formatting:
- Use a Markdown pipe table (`| col | col |`) rather than spacing; pipe tables stay aligned in proportional fonts and print cleanly.

Notes:
- If the chosen baseline is one of the intents (e.g. baseline = Still), the baseline column will usually be blank.
- Some parameters don’t have a clean “more/less” axis (e.g. angle). In those cases, treat `+/-` as a dominance/visibility cue rather than a literal increase/decrease.

---

## Example Tables (Configs 10, 30, 60)

These are “living examples” of the performer-facing output format.

Assumptions:
- Baseline = **No intent** (Intent Strength = 0 or all activations = 0)
- `agency = 0`

### Config 10 — `config/synth/10-palette-smear-av-flow.json`

| Mark \ Intent | Still | Sparse | Flow | Grain | Structure | Turbulence | Maximum |
|---|---|---|---|---|---|---|---|
| Surface: Smear | -- | -- | - | - | -- | + | + |
| Motion: FluidMotion |  |  |  | - |  | + | ++ |
| Accents: SoftMarks |  |  | + |  |  | ++ | ++ |

### Config 30 — `config/synth/30-dualfield-particles-av-inkflow.json`

| Mark \ Intent | Still | Sparse | Flow | Grain | Structure | Turbulence | Maximum |
|---|---|---|---|---|---|---|---|
| Motion: FluidMotion | -- | -- | - | -- | - |  | ++ |
| Particles: ParticleField |  |  | + | ++ | + | ++ | ++ |
| Accents: SoftMarks |  |  | + |  |  | ++ | ++ |
| Overlay: Geometry | + | + | + | ++ | + | ++ | ++ |

### Config 60 — `config/synth/60-chaos-av-dualparticle.json`

| Mark \ Intent | Still | Sparse | Flow | Grain | Structure | Turbulence | Maximum |
|---|---|---|---|---|---|---|---|
| Surface: Smear | -- | -- | -- | - | -- | + | + |
| Motion: FluidMotion | -- | -- | - | - | -- |  | ++ |
| Particles: Webs | -- | -- | -- | + | -- |  |  |
| Accents: SoftMarks | - | - | + |  | - | ++ | ++ |
| Overlay: Geometry | ++ | ++ | ++ | ++ | ++ | ++ | ++ |

---

## Worked example: config 60 (Row 6 chaos A/V dualparticle)

### Accuracy caveat (important)

This delta-table approach can be used immediately, but its accuracy depends on whether each Mod actually uses its `ParamController` outputs.

- If a Mod updates controllers in `applyIntent()` but later reads raw `ofParameter.get()` values (or a downstream simulation reads its own internal parameters), then the table will overestimate intent impact for that Mod.
- Once the intent/controller defects are fixed, re-running the analysis should make particle/fluid-related rows notably more accurate.

A quick way to sanity-check a Mod:
- In `update()`/draw, look for `controller.value` usage (good) vs `parameter.get()` usage (often a red flag).

Config file:
- `config/synth/60-chaos-av-dualparticle.json`

### Major visual subsystems

- Smear layer: `Smear` into `marks` (`config/synth/60-chaos-av-dualparticle.json:296`)
- Two particle webs: `ParticleSetCluster` on `particleset1`, `ParticleSetDisperse` on `particleset2` (`config/synth/60-chaos-av-dualparticle.json:316` and `config/synth/60-chaos-av-dualparticle.json:356`)
- Fluid motion field: `Fluid` + two `FluidRadialImpulse` mods (`config/synth/60-chaos-av-dualparticle.json:216` and `config/synth/60-chaos-av-dualparticle.json:244`)
- Divided-area geometry: `Geometry` (`config/synth/60-chaos-av-dualparticle.json:396`)

### Intent-only caveats specific to config 60

- Many colors are wired from `Palette.* -> <Mod>.Colour` via connections (e.g. `config/synth/60-chaos-av-dualparticle.json:472`). With `agency=0`, those auto-driven colors are effectively ignored.
- `DividedArea` **major line colour is not intent-driven** by design (see `src/sinkMods/DividedAreaMod.cpp:245`). Only the minor line color changes with intent.

### Why D/E/C are “big” in config 60

From the involved `applyIntent()` methods:

- `D` increases video point sampling (`VideoFlowSourceMod`), increases particle alpha (`ParticleSetMod`), and boosts smear alpha multiplier (`SmearMod`). This changes “how much you see”.
- `E` pushes smear toward faster refresh (`SmearMod.mixNew`) and increases impulse strength (weighted with C) (`FluidRadialImpulseMod`). This changes “how fast it evolves”.
- `C` increases smear turbulence (`SmearMod.field2Multiplier` and `jumpAmount`) and contributes to impulse strength. This changes “how wild it is”.

`S` and `G` matter too, but are more about *order* and *scale* (grid/texture sizing, impulse radius).

---

## Checklist template (for repeating on config 61, 62, …)

1. List visible layers (drawn overlays vs base).
2. List major Mods that draw (Smear/Fluid/Particle*/DividedArea/Collage/SandLine/Text/etc.).
3. For each of those Mods: open its `applyIntent()` implementation and list the parameters it updates.
4. Translate each parameter into a performer-relevant visual description.
5. Decide the analysis baseline (Still vs No intent).
6. Build an impact table with intents as columns using `++/+/-/--` (leave blanks for low deltas) as a Markdown pipe table.
7. Optionally add a one-line “look” description per intent.
8. Decide which dimensions are primary/secondary levers for that config.

---

## Optional: what to do if you want an even purer intent experiment

If you want to minimize the baseline manual share for a one-off analysis session, the relevant blending knobs are:

- `ParamControllerSettings::baseManualBias`
- `ParamControllerSettings::manualBiasDecaySec`

These are updated from Synth parameters in `src/core/Synth.cpp:481`.

(Generally for performer intuition you don’t need this; treating `agency=0` + high intent strength as “intent-only” is close enough.)
