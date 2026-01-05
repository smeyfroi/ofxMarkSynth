# Improvisation Grid Design Notes

This note captures decisions for building new MarkSynth improvisation configs (8×7 grid), using the CaldewRiver configs as a tested reference point.

## Core goals

- Provide many configs that are playable and coherent (grounded in live audio/video), not “procedural noise”.
- Use layers as performance registers: keep a stable base, with overlays that can be brought in/out.
- Stay within tested GPU limits (CaldewRiver layer sizes / formats).
- Ensure print-quality output: avoid aliasing/pixelation on hard-edged marks.

## Inputs and "video-only" meaning

- The system always has a mic and a camera connected.
- “Video-only” configs should mean: *drawing/structure responds to video*, while colour can still be audio-derived.
- Therefore, video-only configs can (and usually should) include:
  - `AudioDataSource` + `SomPalette` to learn a colour palette from ambient audio
  - but avoid using audio points/onsets to drive geometry or mark placement

Random sources (`Random*`) are allowed but should be used sparingly; `SomPalette.Random*` taps are fully valid because they are derived from audio.

## Synth defaults (config-wide)

- `synth.agency = 0.1` (start low; can be raised live)
- `initialIntent.strength = 0.75` (headroom for performance)
- Background aesthetic is mostly dark:
  - boot fallback: `backgroundColor = 0, 0, 0, 1`

## Background colour (Pattern A)

- Prefer palette-derived background colour wired into Synth:
  - `SomPalette.Darkest -> .BackgroundColour` for dark-default configs
  - `SomPalette.Lightest -> .BackgroundColour` for configs that can “open up” into lighter space as agency rises

This makes background colour an “agency lane”: raising Synth Agency increases how strongly the palette drives background colour.

## Intents

- Use **7 intents** to match a bank of faders.
- Intents are designed to be *usually exclusive* (one fader active at a time).
- Keep the same 7 intent presets across the whole grid for muscle memory.

Current intent preset set (E/D/S/C/G):

- `Still`:      0.05 / 0.10 / 0.90 / 0.05 / 0.10
- `Sparse`:     0.20 / 0.20 / 0.70 / 0.10 / 0.25
- `Flow`:       0.75 / 0.35 / 0.25 / 0.15 / 0.35
- `Grain`:      0.45 / 0.85 / 0.35 / 0.35 / 0.85
- `Structure`:  0.35 / 0.50 / 0.95 / 0.05 / 0.45
- `Turbulence`: 0.85 / 0.65 / 0.20 / 0.80 / 0.60
- `Maximum`:    0.95 / 0.95 / 0.30 / 0.90 / 0.95

## Agency (auto takeover) and AgencyFactor

- Synth Agency controls how strongly **connected (auto) signals** take over parameters.
- Auto weight is per-parameter and only applies if that parameter has received auto values.
- `AgencyFactor` is a per-Mod scalar for auto takeover (multiplies Synth Agency); it does not currently scale intent strength.

## Fields and two-field consumers

- `Smear` and `ParticleField` can take two fields.
- Field character:
  - `SomPalette.FieldTexture`: fullscreen, all-encompassing “world” field
  - `Fluid.velocitiesTexture`: physically plausible carrier; medium coherence
  - `VideoFlow.FlowField`: localized/detail motion; best as injector into fluid or a secondary field

Recommended consistent routing (when available):
- `Field1Texture` = `Fluid.velocitiesTexture`
- `Field2Texture` = `SomPalette.FieldTexture`

Use `Field1PreScaleExp` / `Field2PreScaleExp` for normalization, and multipliers for artistic mix.

## Layers as performance registers

Two overlay styles are used:

- **Fresh entrance** (default): `paused: true`, `alpha: 0.0`
  - minimal GPU cost until activated
  - starts clean (no buffered events)

- **Latent world** (rare): `paused: false`, very low `alpha`
  - runs continuously so it can accumulate and receive auto values
  - GPU-costly, so limit to 0–1 latent layers per config
  - suggested starting alphas:
    - latent particles: `alpha ~ 0.005–0.02`
    - latent geometry: `alpha ~ 0.02–0.05`

## Layer resolution and format budget (CaldewRiver-guided)

Default `numSamples: 0` everywhere (MSAA is unreliable for DividedAreaMod).

- Substrate / background / fluid values: `2400×2400`, `GL_RGBA32F`
- Velocities: `900×900` (or `1200×1200`), `GL_RG32F`, `isDrawn: false`
- Soft marks: `3600×3600`, `GL_RGBA16F`
- SandLine / ParticleSet: acceptable at `3600` or `5400` depending on prominence
- Hard geometry (DividedArea / crisp edges): `7200×7200`, `GL_RGBA16F`, `numSamples: 0`

Print-quality rule:
- `3600` is OK for inherently soft marks (smear, soft circles, atmospheres).
- Avoid `3600` for hard-edged geometry.

## Controller pad colors (AKAI APC mini)

We need row disambiguation in a dark performance environment.

Proposed scheme:

- Columns (1–8): fixed hue wheel (full spectrum left→right).
- Rows (0–6): vary saturation (vivid → pastel) while keeping value/brightness high.
- Inactive configs are dimmed by the host app; the active config uses the defined color.

Start with these saturation levels (top→bottom rows 0–6):

- Row 0: 1.00
- Row 1: 0.90
- Row 2: 0.80
- Row 3: 0.70
- Row 4: 0.60
- Row 5: 0.50
- Row 6: 0.40

Column hues (degrees): `0, 45, 90, 135, 180, 225, 270, 315`.

Precomputed hex colours (HSV with value=1.0, saturations above):

| row | sat | c1 | c2 | c3 | c4 | c5 | c6 | c7 | c8 |
|---|---|---|---|---|---|---|---|---|---|
| 0 | 1.0 | #FF0000 | #FFBF00 | #80FF00 | #00FF40 | #00FFFF | #0040FF | #8000FF | #FF00BF |
| 1 | 0.9 | #FF1919 | #FFC619 | #8CFF19 | #19FF53 | #19FFFF | #1953FF | #8C19FF | #FF19C6 |
| 2 | 0.8 | #FF3333 | #FFCC33 | #99FF33 | #33FF66 | #33FFFF | #3366FF | #9933FF | #FF33CC |
| 3 | 0.7 | #FF4D4D | #FFD24D | #A6FF4D | #4DFF79 | #4DFFFF | #4D79FF | #A64DFF | #FF4DD2 |
| 4 | 0.6 | #FF6666 | #FFD966 | #B2FF66 | #66FF8C | #66FFFF | #668CFF | #B266FF | #FF66D9 |
| 5 | 0.5 | #FF8080 | #FFDF80 | #BFFF80 | #80FF9F | #80FFFF | #809FFF | #BF80FF | #FF80DF |
| 6 | 0.4 | #FF9999 | #FFE699 | #CCFF99 | #99FFB2 | #99FFFF | #99B2FF | #CC99FF | #FF99E6 |

If saturation differences prove too subtle on the hardware, switch to a small hue offset per row (keep saturation near 1.0 and brightness high).


## Config filenames and load order

Configs are loaded in alpha order, so filenames must start with a numeric prefix.

- Use CaldewRiver-style two-digit prefixes: `01-...json`, `02-...json`, …
- Keep names short but descriptive: `<NN>-<rowTheme>-<modality>-<descriptor>.json`

Example row 1:

- `01-minimal-av-softmarks.json`
- `02-minimal-av-fluidwash.json`
- `03-minimal-av-smeartrails.json`
- `04-minimal-av-geometrylatent.json`
- `05-minimal-audio-cloud.json`
- `06-minimal-audio-impulses.json`
- `07-minimal-video-ink.json`
- `08-minimal-video-field.json`

## Row 1 (first grid row) intent

Row 1 is the “minimal” bank:

- 4 × A/V minimal configs (audio + video)
- 2 × audio-driven (drawing responds to audio)
- 2 × video-driven (drawing responds to video; colour may still be audio-derived)

Optionally include a latent geometry layer in 1–2 of these configs as an “expand” register.

## Full 8×7 grid numbering

Configs load in filename alpha order, so we use two-digit numeric prefixes that map directly to the grid.

- Row 1: `01`–`08`
- Row 2: `09`–`16`
- Row 3: `17`–`24`
- Row 4: `25`–`32`
- Row 5: `33`–`40`
- Row 6: `41`–`48`
- Row 7: `49`–`56`

## Proposed row themes (rows 2–7)

These are designed so each row is an "instrument family" and columns remain the modality variants (A/V vs audio-driven vs video-driven).

- Row 2: `palette-smear` — SomPalette as global style; Smear as the main surface.
- Row 3: `fluid-carriers` — Fluid as substrate; camera/audio impulses inject velocity; downstream marks ride the carrier.
- Row 4: `dualfield-particles` — ParticleField as main voice (Field1=fluid velocities, Field2=SomPalette field).
- Row 5: `geometry-architecture` — DividedArea and structured paths; mostly fresh-entrance overlays (high res).
- Row 6: `memory-collage` — Memory bank + Collage; optional text remains commented out.
- Row 7: `chaos-bank` — denser configs, but still organized into layers and registers; use fresh-entrance overlays heavily to protect GPU.

## Per-row layer budgeting (rule of thumb)

- Keep a stable base (often `2400×2400` RGBA32F).
- Add at most one additional always-running heavy process (Fluid or Smear or ParticleField).
- Use overlays for complexity, mostly paused.
- Avoid multiple always-running `7200×7200` layers.
