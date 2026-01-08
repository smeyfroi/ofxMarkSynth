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

- `synth.agency = 0.20` (background colour reads more immediately; can still be raised live)
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

Row 3 (CaldewRiver-informed) variants that intentionally increase complexity:
- Use `VideoFlow.FlowField` as `Field2Texture` for `ParticleField` (choreographic handwriting), especially in A/V + video columns.
- Use different fields (or different multipliers) for `Smear` vs `ParticleField` so they don’t feel redundant.

Use `Field1PreScaleExp` / `Field2PreScaleExp` for normalization, and multipliers for artistic mix.
- `ParticleField.speedThreshold` effectively fades out slow particles (lower = stricter gating; higher = more haze).

## Mark seeding and layer processors

- Every config must draw marks somewhere (no “only post-processing” patches).
- `Smear` and `Fade` only have a visible effect if their target drawing layer is seeded with marks.
- Avoid stacking multiple layer processors on the same drawing layer (especially `Fade` + `Smear`).
- Do not apply `Fade` to a `Fluid` values layer; use `Fluid.Value Dissipation` for decay.
  - When using `Smear` as the persistence mechanism, tune `Smear.MixNew` and `Smear.AlphaMultiplier` (and/or the multiplier sinks) instead of adding `Fade`.
- `ParticleField` does not decay by itself: it must draw into a layer with an explicit persistence mechanism (`Fade`, `Smear`, `Fluid`, or `clearOnUpdate: true`).
- A useful higher-complexity pattern is **two ParticleFields**:
  - `ParticleInk` draws into the `Fluid` values layer (dye injection), and decays via `Fluid.Value Dissipation` (no `Fade` on that layer).
  - A separate foreground `ParticleField` draws into a dedicated `particlefield` layer with its own persistence (either `Fade` or `Smear`).
  - For “latent ink”, keep the wash visible and instead lower `ParticleInk.PointColour.a` and/or lower `ParticleInk.speedThreshold`.
- `DividedArea` can split outputs across layers: route minor lines via `layers.default` and major lines via `layers.major-lines`.

### DividedArea major-lines layer routing

Major lines can target either **overlay** or **non-overlay** layers, with different capabilities:

**Overlay layers** (typical pattern):
- Draw in `drawOverlay()` with access to the composite FBO
- Supports all `majorLineStyle` values including refractive/chromatic styles
- Use for dramatic, wide refractive lines that interact with the background
- Typical config: `geometry` layer at 7200×7200, `isOverlay: true`

**Non-overlay layers** (experimental pattern):
- Draw in `update()` without background FBO access
- Only supports background-free styles: `Solid` (0), `InnerGlow` (1), `BloomedAdditive` (2), `Glow` (3)
- If a background-requiring style is configured, falls back to `Solid` with a warning
- Use for thin major lines that become dye in fluid or marks in a smeared layer
- Typical config: route `major-lines` to `ground` (fluid values layer) with `majorLineStyle: 0` and smaller `MajorLineWidth` (~30–60)

**Major line style reference** (`majorLineStyle` values):
- `0` = Solid (no background needed)
- `1` = InnerGlow (no background needed)
- `2` = BloomedAdditive (no background needed)
- `3` = Glow (no background needed)
- `4` = Refractive (requires background FBO)
- `5` = BlurRefraction (requires background FBO)
- `6` = ChromaticAberration (requires background FBO)

### DividedArea geometry layer fade rates

When major lines target a dedicated overlay layer with `Fade`, the fade rate determines trail behavior:

| Alpha | Behavior | Use case |
|-------|----------|----------|
| 0.030–0.040 | Fast/responsive | Lines appear and disappear quickly; clean, immediate feel |
| 0.015–0.025 | Medium | Balanced; lines linger briefly then fade |
| 0.008–0.012 | Medium-slow | Noticeable trails; lines leave ghostly traces |
| 0.003–0.006 | Slow/trailing | Long persistence; builds up a history of line positions |

Mix these across configs for variety. "Lace" and "weave" themed configs suit slower fades; "ink" and "drive" themes suit faster response.

- `clearOnUpdate: true` layers clear every frame: they are valid targets for mark-making Mods (including `ParticleField`), but are not valid targets for `Fade`, `Smear`, or `Fluid`.
- Fluid usage patterns (both valid and useful):
  - **Visible wash**: a drawn values layer seeded with marks + velocity injection.
  - **Velocity-only carrier**: a small, non-drawn values layer + velocity injection; use `Fluid.velocitiesTexture` downstream.
- `Fluid.Vorticity` is safe and musically useful across the full 0–100 range (0 = simpler, 100 = more intricate).
- Seeding floor pattern: instead of wiring `Audio.RmsScalar` directly to a mark’s `AlphaMultiplier`, use a `MultiplyAdd` mapper to provide a consistent baseline (e.g. `out = 0.25 * rms + 0.05`).
- Fluid velocity injection defaults:
  - **Micro-injection (safe; good for velocity-only carriers feeding Smear/marks)**: smaller radius + low strength + moderate `dt`.
    - Camera baseline example: `radius≈0.02`, `strength≈0.10`, `dt≈0.26`.
    - Audio baseline example (a bit stronger): `radius≈0.03`, `strength≈0.18`, `dt≈0.25`.
  - **Carrier-forward (good for visible wash rows)**: larger radius/strength + smaller `dt` (e.g. `radius≈0.05–0.07`, `strength≈0.24–0.35`, `dt≈0.12`).
  - Sanity heuristic when tuning: `strength * radius^2 / dt` (smaller = safer).
- Decay baseline: many patches read better with slower `Fade` than the early defaults.
  - Marks-only layers: start around `FadeMarks.Alpha ≈ 0.0013–0.0016`.
  - Accents layers: start around `FadeAccents.Alpha ≈ 0.0010–0.0012`.
  - Use faster fades as an intentional “evaporation” choice.
- Intent mapping trick: to invert a normalized signal (`out = 1 - in`), use `MultiplyAdd` with `Multiplier = -1` and `Adder = 1`.

### Smear parameter guidelines

SmearMod requires careful tuning to produce visible results:

- **MixNew**: Controls blend between new content and smeared history.
  - `0.85–0.95`: Subtle smearing, content mostly stays in place
  - `0.65–0.80`: Moderate smearing with visible trails
  - `0.40–0.60`: Heavy smearing, content drifts significantly
  - Too high (>0.95) makes smear nearly invisible; too low (<0.40) causes content to wash out

- **Field multipliers**: Must be high enough to produce visible motion.
  - `Field1Multiplier` / `Field2Multiplier`: Start around `0.5–0.8`
  - Values below `0.1` produce almost no visible smearing
  - When using two fields, balance their multipliers (e.g., `0.6` + `0.4`)

- **AlphaMultiplier**: Controls persistence/decay.
  - `0.994–0.998`: Long persistence, content accumulates
  - `0.990–0.993`: Medium persistence
  - Below `0.990`: Content fades quickly

- **GridSize**: Maximum is `48, 48` (parameter limit).
  - For Strategy 0 (standard): `512, 512` is typical
  - For Strategy 6 (Voronoi): `48, 48` (max) gives finest cells

- **Field connections**: Always connect field textures when using non-zero multipliers.
  - `Fluid.velocitiesTexture` or `Camera.FlowField` for Field1
  - `Palette.FieldTexture` for Field2 (provides audio-reactive motion)

### Wash and seed layer visibility

Soft seeding marks (SoftCircle "Mist" or "Wash") need sufficient visibility to read against the background:

- **AlphaMultiplier**: Start around `0.12–0.20` for visible wash; `0.03–0.06` is often too faint
- **Colour**: Use `0.15–0.20` gray base (e.g., `0.18, 0.18, 0.18, 1`) rather than very dark values
- **Radius**: `0.08–0.14` provides good coverage without being too blobby
- **Fade Alpha**: Use slow fade (`0.0003–0.0006`) for long persistence; faster fade (`0.0010–0.0015`) causes marks to disappear quickly

### Latent layer visibility

For overlay layers that should be subtly visible from the start (rather than fresh-entrance):

- Use `paused: false` with `alpha: 0.10` (10%) for subtle background activity
- This adds visual complexity without overwhelming the primary content
- Particularly effective for ParticleSet webs and Collage fills
- GPU cost is higher than fresh-entrance, so limit to 1–2 latent overlays per config

### Config differentiation strategies

When configs in a row look too similar, vary these parameters:

**DividedArea differentiation:**
- `majorLineStyle`: 0 (solid), 4 (refractive), 5 (blur), 6 (chromatic) — each has distinct character
- `MajorLineWidth`: 150–200 (architectural) vs 300–400 (dramatic)
- `unconstrainedSmoothness`: 0.25–0.35 (angular) vs 0.55–0.70 (organic curves)
- `FadeGeometry Alpha`: 0.008–0.015 (trailing) vs 0.030–0.045 (responsive)

**ParticleSet differentiation:**
- `attractionStrength`: positive (clustering) vs negative (dispersing)
- `connectionRadius`: 0.05–0.08 (tight webs) vs 0.12–0.18 (loose webs)
- `maxParticles`: 800–900 (sparse) vs 1100–1300 (dense)
- `maxParticleAge`: 100–130 (short-lived, responsive) vs 180–220 (long trails)

### Duplicate mods pattern

A single config can instantiate the same Mod type multiple times with different names, parameters, layer targets, and input connections. This creates richer visual complexity without requiring new Mod types.

**Why use duplicate mods:**
- **Contrasting behaviors**: Two ParticleSets with opposite attraction (clustering vs dispersing) create visual tension
- **Layer separation**: Same visual element on different layers with different persistence/processing
- **Input variety**: Same Mod type driven by different sources (audio vs video, clusters vs raw points)
- **Density layering**: Sparse + dense versions of the same element for depth

**Naming convention:**
Use descriptive suffixes that indicate the behavioral difference:
- `ParticleSetCluster` / `ParticleSetDisperse` (behavior)
- `SandLineSparse` / `SandLineDense` (density)
- `MistWide` / `MistTight` (scale)
- `ParticleInk` / `ParticleField` (layer target: fluid dye vs foreground)

**ParticleSet duplicate patterns:**

| Variant | attractionStrength | connectionRadius | maxParticles | Character |
|---------|-------------------|------------------|--------------|-----------|
| Cluster | +0.03 to +0.06 | 0.08–0.10 | 900–1100 | Tight webs, gravitating motion |
| Disperse | -0.03 to -0.06 | 0.12–0.16 | 700–900 | Loose webs, expanding motion |

Different point sources add variety:
- Cluster variant: `Camera.PointVelocity` (video-driven, includes velocity)
- Disperse variant: `Audio.PolarPitchRmsPoint` (audio-driven, position only)

Different color sources:
- Cluster variant: `Palette.RandomLight` (brighter webs)
- Disperse variant: `Palette.Random` or `Palette.RandomDark` (varied/darker webs)

**SoftCircle duplicate patterns:**

| Variant | Radius | AlphaMultiplier | Target Layer | Purpose |
|---------|--------|-----------------|--------------|---------|
| Mist | 0.10–0.14 | 0.08–0.12 | marks/smear | Background seeding, soft atmosphere |
| Glow | 0.04–0.06 | 0.18–0.25 | ground/accents | Accent marks, visible punctuation |
| Seeds | 0.06–0.08 | 0.10–0.14 | fluid values | Dye injection for fluid |

**SandLine duplicate patterns:**

| Variant | Density | PointRadius | StdDevPerpendicular | Character |
|---------|---------|-------------|---------------------|-----------|
| Sparse | 0.08–0.12 | 16–24 | 0.02–0.03 | Bold, sparse grain |
| Dense | 0.25–0.35 | 6–10 | 0.01–0.015 | Fine, textural grain |

**ParticleField duplicate patterns (Row 3 style):**

| Variant | Target Layer | PointColour.a | speedThreshold | Purpose |
|---------|--------------|---------------|----------------|---------|
| ParticleInk | ground (fluid) | 0.04–0.06 | 0.8–1.0 | Dye injection, decays via fluid |
| ParticleField | particlefield | 0.5–0.7 | 1.5–2.5 | Foreground particles, own Fade |

**FluidRadialImpulse duplicate patterns:**

| Variant | Source | Impulse Radius | Impulse Strength | Character |
|---------|--------|----------------|------------------|-----------|
| CameraImpulse | CameraClusters | 0.05–0.07 | 0.24–0.30 | Broad, smooth flow |
| AudioImpulse | AudioClusters | 0.03–0.05 | 0.18–0.24 | Focused, rhythmic bursts |

**Connection patterns for duplicates:**

Each duplicate needs its own connections. Wire to different sources for variety:
```json
"connections": [
  "Camera.PointVelocity -> ParticleSetCluster.PointVelocity",
  "Palette.RandomLight -> ParticleSetCluster.Colour",

  "Audio.PolarPitchRmsPoint -> ParticleSetDisperse.Point",
  "Palette.Random -> ParticleSetDisperse.Colour"
]
```

**Layer targeting for duplicates:**

Each duplicate typically targets a different layer:
```json
"ParticleSetCluster": {
  "layers": { "default": ["particleset1"] }
},
"ParticleSetDisperse": {
  "layers": { "default": ["particleset2"] }
}
```

Or the same layer for additive complexity:
```json
"MistWide": {
  "layers": { "default": ["marks"] }
},
"MistTight": {
  "layers": { "default": ["marks"] }
}
```

**GPU considerations:**
- Each duplicate Mod instance has full processing overhead
- ParticleSet duplicates are expensive (physics simulation per instance)
- SoftCircle/SandLine duplicates are lightweight
- Limit to 2–3 duplicates of expensive Mods per config
- Use lower `maxParticles` on secondary ParticleSet instances

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

A pragmatic third pattern is a **shared optional register pack** used across multiple rows/configs:

- A mid/high-res `collage` fill layer (e.g. `5400×5400`) with `useStencil: true`.
- A high-res `geometry` layer (`7200×7200`) that can host *both* DividedArea lines and Collage outlines.

Nuances when sharing a geometry layer:

- Collage outlines are “once-and-done” draws (they only appear when a new collage path is emitted), so they need persistence (`clearOnUpdate: false`) and usually a slow `FadeGeometry`.
- DividedArea tends to redraw continuously; on a shared layer with `FadeGeometry`, the DividedArea content stays present via redraw while outlines decay.
- If the shared geometry layer is driven by continuous processors (e.g. Smear/Fluid/clearOnUpdate), outlines behave according to that layer’s persistence rules (which can be artistically fine, but should be a deliberate choice).

## Layer resolution and format budget (CaldewRiver-guided)

Default `numSamples: 0` everywhere (MSAA is unreliable for DividedAreaMod).

- Substrate / background / fluid values: `2400×2400`, `GL_RGBA32F`
- Velocities: `900×900` (or `1200×1200`), `GL_RG32F`, `isDrawn: false`
- Soft marks: `3600×3600`, `GL_RGBA16F`
- SandLine: acceptable at `3600` or `5400` depending on prominence
- Collage fill: `5400×5400`, `GL_RGBA16F` with `useStencil: true` (CollageMod uses stencil for snapshot clipping)
- ParticleSet: if using connection lines, treat as hard geometry and prefer `7200×7200` (it draws crisp line segments between nearby particles)
- Hard geometry (DividedArea / crisp edges / ParticleSet lines / collage outlines): `7200×7200`, `GL_RGBA16F`, `numSamples: 0`

Print-quality rule:
- `3600` is OK for inherently soft marks (smear, soft circles, atmospheres).
- Avoid `3600` for hard-edged geometry.

### Layer resolution for DividedArea and other hard-edged marks

DividedArea minor lines are especially prone to aliasing because they are thin (typically `PathWidth: 2`) and draw many constrained line segments. When these lines are smeared, aliasing artifacts become more visible as the content moves.

**Resolution guidelines:**

| Content Type | Minimum Resolution | Notes |
|--------------|-------------------|-------|
| Soft marks only (SoftCircle, fluid dye) | 3600×3600 | OK for inherently soft content |
| DividedArea minor lines | 7200×7200 | Thin lines alias badly at lower resolutions |
| ParticleSet connection lines | 7200×7200 | Crisp line segments need high res |
| DividedArea + Smear | 7200×7200 | Smear amplifies aliasing artifacts |
| SandLine | 3600×3600 | Inherently grainy; aliasing less visible |
| Collage outlines | 7200×7200 | Draw to high-res geometry layer |

**When DividedArea draws minor lines to a layer:**
- Use 7200×7200 for that layer, even if it also hosts soft marks
- The soft marks don't need high resolution, but DividedArea does
- This is especially important when Smear is applied to the same layer

**GPU budget consideration:**
- A 7200×7200 GL_RGBA16F layer uses ~400MB VRAM
- This is acceptable for 1-2 such layers per config
- Avoid multiple always-running 7200×7200 layers

## Controller pad colors (AKAI APC mini)

We need row disambiguation in a dark performance environment, while also encoding modality on the grid.

Scheme:

- Columns (0–7) are split into 3 modality chunks:
  - 0–3: A/V (warm hues)
  - 4–5: audio-driven (green/cyan)
  - 6–7: video-driven (blue/purple)
- Rows (0–6) vary saturation (vivid → pastel) while keeping value/brightness high.
- Inactive configs are dimmed by the host app; the active config uses the defined color.

Row saturations (top→bottom rows 0–6):

- Row 0: 1.00
- Row 1: 0.90
- Row 2: 0.80
- Row 3: 0.70
- Row 4: 0.60
- Row 5: 0.50
- Row 6: 0.40

Column hues (degrees): `0, 20, 40, 60, 120, 160, 220, 270`.

Precomputed hex colours (HSV with value=1.0, saturations above):

| row | sat | c0 | c1 | c2 | c3 | c4 | c5 | c6 | c7 |
|---|---|---|---|---|---|---|---|---|---|
| 0 | 1.0 | #FF0000 | #FF5500 | #FFAA00 | #FFFF00 | #00FF00 | #00FFAA | #0055FF | #8000FF |
| 1 | 0.9 | #FF1919 | #FF6619 | #FFB219 | #FFFF19 | #19FF19 | #19FFB2 | #1966FF | #8C19FF |
| 2 | 0.8 | #FF3333 | #FF7733 | #FFBB33 | #FFFF33 | #33FF33 | #33FFBB | #3377FF | #9933FF |
| 3 | 0.7 | #FF4D4D | #FF884D | #FFC34D | #FFFF4D | #4DFF4D | #4DFFC3 | #4D88FF | #A64DFF |
| 4 | 0.6 | #FF6666 | #FF9966 | #FFCC66 | #FFFF66 | #66FF66 | #66FFCC | #6699FF | #B266FF |
| 5 | 0.5 | #FF8080 | #FFAA80 | #FFD480 | #FFFF80 | #80FF80 | #80FFD4 | #80AAFF | #BF80FF |
| 6 | 0.4 | #FF9999 | #FFBB99 | #FFDD99 | #FFFF99 | #99FF99 | #99FFDD | #99BBFF | #CC99FF |

## Config filenames and load order

Configs are loaded in alpha order, so filenames must start with a numeric prefix. We use a two-digit prefix that maps directly to controller grid location.

- Prefix format: `RC` where `R` is row (0–6) and `C` is column (0–7).
- Full name format: `<RC>-<rowTheme>-<modality>-<descriptor>.json`

Examples (rows 0–1 already built):

- `00-minimal-av-softmarks.json`
- `07-minimal-video-field.json`
- `10-palette-smear-av-flow.json`
- `17-palette-smear-video-geometry-text.json`

## Row 0 (minimal bank)

Columns are reserved consistently across rows:

- Cols 0–3: A/V (audio + video)
- Cols 4–5: audio-driven (drawing responds to audio)
- Cols 6–7: video-driven (drawing responds to video; colour may still be audio-derived)

Optionally include a latent geometry layer in 0–1 of these configs as an “expand” register.

## Row 1 (palette-smear bank)

SomPalette is the global style; Smear is the main surface.

Notes:

- Ensure the smeared marks layer is always seeded (e.g. a low-alpha “Mist” SoftCircle that runs continuously).
- Avoid stacking `Fade` on the smeared marks layer; tune `Smear.MixNew` / `Smear.AlphaMultiplier` instead.
- Add a non-smeared `accents` layer for a crisp foreground voice; it can have its own `FadeAccents`.
- Optional text can be kept in configs as underscore-prefixed mods/keys and `"_connections_text"`.

## Full 8×7 grid numbering

- Row 0: `00`–`07`
- Row 1: `10`–`17`
- Row 2: `20`–`27`
- Row 3: `30`–`37`
- Row 4: `40`–`47`
- Row 5: `50`–`57`
- Row 6: `60`–`67`

## Proposed row themes (rows 2–6)

These are designed so each row is an "instrument family" and columns remain the modality variants.

- Row 2: `fluid-carriers` — Fluid as substrate; camera/audio impulses inject velocity; downstream marks ride the carrier.
- Row 3: `dualfield-particles` — Two ParticleFields (ink into fluid + foreground particles); DividedArea with split layer routing.
- Row 4: `geometry-architecture` — DividedArea and structured paths; mostly fresh-entrance overlays (high res).
- Row 5: `memory-collage` — Memory bank + Collage; optional text remains commented out.
- Row 6: `chaos-bank` — denser configs, but still organized into layers and registers; use fresh-entrance overlays heavily to protect GPU.

## Row 3 (dualfield-particles bank)

Two ParticleFields per config with DividedArea geometry:

**Layer structure:**
- `ground` (2400×2400, RGBA32F): Fluid values layer; receives `ParticleInk` and sometimes DividedArea minor lines
- `velocities` (900×900, RG32F): Fluid velocity carrier (not drawn)
- `particlefield` (3600×3600, RGBA16F): Foreground particle register; receives `ParticleField` and sometimes DividedArea minor lines
- `accents` (3600×3600, RGBA16F): SoftCircle accents with own Fade
- `geometry` (7200×7200, RGBA16F, overlay): DividedArea major lines (most configs)

**ParticleField patterns:**
- `ParticleInk`: draws into `ground` (fluid values layer); decays via `Fluid.Value Dissipation`
  - "Visible ink": higher `PointColour.a` (~0.06), moderate `speedThreshold` (~0.9–1.0)
  - "Latent ink": lower `PointColour.a` (~0.03), lower `speedThreshold` (~0.6–0.7) for motion-gating
- `ParticleField`: draws into `particlefield` register; decays via `Fade` or `Smear`
  - Higher `speedThreshold` (~2.2–2.8) for more selective particle visibility

**DividedArea patterns:**
- Minor lines (`layers.default`): route to `ground` or `particlefield` depending on desired texture
- Major lines (`layers.major-lines`): typically route to `geometry` overlay with refractive styles
  - Exception: config 33 (`architect`) routes major lines to `ground` with `majorLineStyle: 0` (Solid) for thin dye lines in fluid

**Smear variants (configs 32, 36):**
- `particlefield` layer uses `Smear` instead of `Fade`
- Smear driven by `Camera.FlowField` (Field1) + `Fluid.velocitiesTexture` (Field2)
- No `Fade` on smeared layer; tune `Smear.MixNew` and `Smear.AlphaMultiplier` for persistence

**Geometry layer fade rates (varied for character):**

| Config | Name | FadeGeometry Alpha | Character |
|--------|------|-------------------|-----------|
| 30 | inkflow | 0.035 | Fast/responsive |
| 31 | weave | 0.008 | Slower trails |
| 32 | smearflow | 0.020 | Medium |
| 33 | architect | — | No geometry layer (major lines → ground) |
| 34 | inkdrive | 0.030 | Fast |
| 35 | latentlace | 0.004 | Long trails |
| 36 | smearflow | 0.015 | Medium-slow |
| 37 | inkdelta | 0.012 | Medium-slow |

**MajorLineWidth values:**
- Overlay configs: 220–300 (wide refractive lines)
- Non-overlay config (33): 40 (thin solid lines as fluid dye)

## Row 4 (geometry-architecture bank)

DividedArea as primary voice with ParticleSet connection-line webs as contrasting organic element. Smear/Fluid available as optional secondary layers for continuity with earlier rows.

**Design goals:**
- Architectural/geometric aesthetic as the primary visual identity
- ParticleSet webs provide contrasting organic complexity (first row to foreground ParticleSet)
- Responsive major lines (faster fade ~0.025–0.035)
- Soft seeding layer with long persistence ensures content for smear/marks
- Collage complements the architectural aesthetic in select configs

**Layer structure:**

| Layer | Size | Format | Purpose | Notes |
|-------|------|--------|---------|-------|
| `ground` | 2400×2400 | GL_RGBA32F | Fluid values | Optional (4 configs) |
| `velocities` | 900×900 | GL_RG32F | Fluid velocity | Optional (4 configs) |
| `marks` | 7200×7200 | GL_RGBA16F | DividedArea minor lines + soft seed marks | High res required for DividedArea |
| `particleset` | 7200×7200 | GL_RGBA16F | ParticleSet webs | Fresh-entrance; some configs draw to other layers |
| `geometry` | 7200×7200 | GL_RGBA16F, overlay | DividedArea major lines | Always present |
| `collage` | 5400×5400 | GL_RGBA16F, useStencil | Collage fills | Fresh-entrance (2 configs) |

**Config matrix:**

| # | Name | Modality | Fluid | ParticleSet Target | Collage | DividedArea Minor Seeds |
|---|------|----------|-------|-------------------|---------|------------------------|
| 40 | `geometry-av-lines` | A/V | No | `particleset` + Fade | No | Direct points (higher complexity) |
| 41 | `geometry-av-fluidweb` | A/V | Yes | `ground` (fluid dye) | No | Clusters (lower complexity) |
| 42 | `geometry-av-collage` | A/V | No | `particleset` + Fade | Yes | Clusters |
| 43 | `geometry-av-smearweb` | A/V | Yes | `marks` (smeared) | No | Direct points |
| 44 | `geometry-audio-lines` | Audio | No | `particleset` + Fade | No | Clusters |
| 45 | `geometry-audio-fluid` | Audio | Yes | `ground` (fluid dye) | Yes | Clusters |
| 46 | `geometry-video-lines` | Video | No | `particleset` + Fade | No | Direct points |
| 47 | `geometry-video-smear` | Video | Yes | `marks` (smeared) | No | Direct points |

**DividedArea configuration:**
- Major anchors: always from clusters (`ClusterMod.ClusterCentre`)
- Minor anchors: `Audio.PolarPitchRmsPoint` (provides audio-reactive texture in all columns)
- `majorLineStyle`: 4/5/6 (refractive styles)
- `MajorLineWidth`: 200–350
- `unconstrainedSmoothness`: 0.3–0.7
- Geometry layer fade: 0.025–0.035 (responsive)

**ParticleSet configuration:**
- `strategy`: 2 (connections and points)
- `maxParticles`: 800–1200
- `maxParticleAge`: 100–200
- `connectionRadius`: 0.08–0.15
- `colourMultiplier`: 0.5–1.0
- `attractionStrength`: -0.05 to 0.03
- Point source: `Camera.PointVelocity` (A/V, Video), `Audio.PolarPitchRmsPoint` (Audio)

**ParticleSet layer variations:**
- **Dedicated `particleset` layer (Fade persistence)**: 7200×7200, `paused: true`, `alpha: 0.0`, `FadeParticleSet.Alpha`: 0.002–0.004
- **Drawing to `ground` (fluid dye)**: lower `particleDrawRadius` (1–2), lower color alpha (~0.4–0.6), decays via `Fluid.Value Dissipation`
- **Drawing to `marks` (smeared)**: higher `colourMultiplier` (0.7–1.0), smear fields from `Fluid.velocitiesTexture` + `Palette.FieldTexture`

**Seeding layer:**
- SoftCircle "Mist" with very low alpha on `marks` layer
- Long persistence via slow Fade (~0.0008–0.0012)
- Ensures smear/marks layer always has content

**Collage (configs 42, 45):**
- PathMod with `TriggerBased: true`, triggered by `Audio.Onset1`
- Points from clusters
- Memory bank or PixelSnapshot for texture
- Outlines to shared `geometry` layer

## Row 5 (memory-collage bank)

CollageMod as primary voice with Memory bank textures. Memory bank is pre-seeded automatically; configs wire `Synth.Memory` → `CollageMod.SnapshotTexture` and use audio onsets/timbre/pitch changes to trigger memory emission.

**Design goals:**
- Collage as the central visual element across all configs
- Memory textures provide photographic/captured material for collage fills
- Variety through different secondary elements: DividedArea, Fluid, Smear, SandLine, ParticleSet
- Simpler layer structures than Rows 3–4 (collage is GPU-intensive with stencil operations)

**Layer structure:**

| Layer | Size | Format | Purpose | Notes |
|-------|------|--------|---------|-------|
| `wash` | 4800×4800 | GL_RGBA16F | Palette-derived soft marks | Optional background texture |
| `collage` | 4800×4800 | GL_RGBA16F, useStencil | Collage fills | Always present; stencil required |
| `fluid` | 4800×4800 | GL_RGBA32F, useStencil | Fluid values + collage | Alternative to separate collage layer |
| `smear` | 4800×4800 | GL_RGBA16F, useStencil | Smeared collage | For configs with field-based smearing |
| `sand` | 4800×4800 | GL_RGBA16F | SandLine traces | Textural borders (config 53) |
| `geometry` | 7200×7200 | GL_RGBA16F, overlay | DividedArea + outlines | Fresh-entrance (config 50) |
| `particles` | 7200×7200 | GL_RGBA16F | ParticleSet webs | Fresh-entrance (config 57) |
| `velocities` | 900×900 | GL_RG32F | Fluid velocity | Not drawn (fluid/smear configs) |

**Config matrix:**

| # | Name | Modality | Secondary Element | Memory Trigger | Notes |
|---|------|----------|-------------------|----------------|-------|
| 50 | `collage-av-geometric` | A/V | DividedArea (solid major lines) | `Onset1` | Architectural; geometry overlay |
| 51 | `collage-av-fluidwash` | A/V | Fluid (high vorticity) | `TimbreChange` | Organic shapes in fluid |
| 52 | `collage-av-smearfield` | A/V | Smear (Voronoi strategy 6) | `Onset1` | Field-based smear variety |
| 53 | `collage-av-sandtrace` | A/V | SandLine | `Onset1` | Textural/grainy borders |
| 54 | `collage-audio-bold` | Audio | None (high contrast fills) | `TimbreChange` | Bold thick outlines |
| 55 | `collage-audio-smear` | Audio | Smear (fluid velocities) | `TimbreChange` | Low MixNew for trails |
| 56 | `collage-video-fluid` | Video | Fluid | `PitchChange` | Video paths in fluid |
| 57 | `collage-video-particle` | Video | ParticleSet webs | `PitchChange` | Connection lines contrast |

**Memory bank wiring pattern:**
```
Audio.[Onset1|TimbreChange|PitchChange] → Synth.MemoryEmit[Random|RandomNew|RandomOld]
Synth.Memory → Collage.SnapshotTexture
```

**CollageMod configuration:**
- `Colour`: neutral gray base (~0.5, 0.5, 0.5, 0.6–0.7), tinted by palette
- `Saturation`: 1.6–1.8 (enhances memory texture colors)
- `Strategy`: 1 (tinted snapshot) or 2 (palette-blended)
- `OutlineAlphaFactor`: 0.0 (no outlines) to 0.5 (visible outlines)
- `OutlineWidth`: 12–18 (when outlines enabled)

**PathMod configuration:**
- `Strategy`: 0–3 (varied path generation)
- `ClusterRadius`: 0.08–0.15
- `MaxVertices`: 6–10
- `TriggerBased`: false (continuous path generation)

**DividedArea (config 50):**
- `majorLineStyle`: 0 (Solid) — architectural feel without refractive complexity
- `MajorLineWidth`: 200
- Minor lines to `wash`, major lines to `geometry` overlay
- Geometry fade: 0.025 (responsive)

**Fluid configs (51, 55, 56):**
- `Vorticity`: 55–65
- `Value Dissipation`: 0.9988–0.9990
- Collage draws directly to fluid values layer (no separate collage layer)

**Smear configs (52, 55):**
- `MixNew`: 0.45–0.55 (lower = more trails)
- `AlphaMultiplier`: 0.996
- Field1 from `Fluid.velocitiesTexture` or `Camera.FlowField`

**ParticleSet (config 57):**
- `strategy`: 2 (connections and points)
- `maxParticles`: 900
- `connectionRadius`: 0.08
- Fresh-entrance overlay (paused, alpha 0.0)
- Provides organic contrast to geometric collage shapes

## Row 6 (chaos-bank)

Maximum density configurations with 5–6 visual elements per config. Every config includes ParticleSet. Uses sparse DividedArea drawn into fluid/smear layers rather than dedicated geometry overlays. Duplicate mods (two ParticleSets, two SoftCircles, two SandLines) for variety. Multiple latent layers at low alpha.

**Design goals:**
- Maximum visual density while maintaining performance
- ParticleSet in every config (proven versatile in Rows 4–5)
- Sparse DividedArea as background texture (not primary focus)
- Duplicate mods with contrasting configurations
- Brighter overall aesthetic (`backgroundMultiplier: 0.33–0.35`)
- Collage used sparingly (only config 63)
- Fresh-entrance overlays to protect GPU

**Layer structure:**

| Layer | Size | Format | Purpose | Notes |
|-------|------|--------|---------|-------|
| `ground` | 2400×2400 | GL_RGBA32F | Fluid values | Receives DividedArea minor lines, SoftCircle dye |
| `velocities` | 900×900 | GL_RG32F | Fluid velocity | Not drawn |
| `marks` | 7200×7200 | GL_RGBA16F | DividedArea minor lines + soft marks | High res for lines |
| `particleset` | 7200×7200 | GL_RGBA16F | ParticleSet webs | Primary particle layer |
| `particleset2` | 7200×7200 | GL_RGBA16F | Second ParticleSet | Contrasting behavior |
| `collage` | 5400×5400 | GL_RGBA16F, useStencil | Collage fills | Only config 63 |
| `geometry` | 7200×7200 | GL_RGBA16F, overlay | DividedArea major lines | Optional overlay |

**Config matrix:**

| # | Name | Modality | Primary Elements | Secondary Elements | Latent Layers |
|---|------|----------|-----------------|-------------------|---------------|
| 60 | `chaos-av-dualparticle` | A/V | Two ParticleSets (cluster + disperse), Fluid | Sparse DividedArea→fluid, Smear | particleset2 @ 10% |
| 61 | `chaos-av-flowlines` | A/V | Fluid, ParticleSet, sparse DividedArea→fluid | SandLine, dual SoftCircles | marks @ 8% |
| 62 | `chaos-av-weave` | A/V | Smear, ParticleSet, ParticleField (ink + foreground) | DividedArea, SoftCircle | particlefield @ 12% |
| 63 | `chaos-av-collage` | A/V | Fluid, ParticleSet, DividedArea, Collage | SoftCircle seed | collage @ 10% |
| 64 | `chaos-audio-dense` | Audio | Smear, two ParticleSets | DividedArea→smear, SandLine | particleset2 @ 8% |
| 65 | `chaos-audio-layers` | Audio | Fluid, ParticleSet, sparse DividedArea→fluid | Dual SoftCircles | marks @ 10% |
| 66 | `chaos-video-web` | Video | Fluid, ParticleSet, DividedArea | Smear, two Fades | geometry @ 12% |
| 67 | `chaos-video-sand` | Video | Smear, ParticleSet, DividedArea | Two SandLines (sparse + dense) | sand @ 10% |

**DividedArea configuration (sparse):**
- `maxConstrainedLines`: 250–350 (much lower than previous rows)
- `PathWidth`: 2 (thin minor lines)
- Minor lines target `ground` (fluid dye) or `marks` (smeared) — not dedicated overlay
- Major lines: mostly disabled or thin solid style (`majorLineStyle: 0`)
- `maxUnconstrainedLines`: 2–4 (very few major lines when enabled)

**ParticleSet dual-particle patterns:**
- **Cluster ParticleSet**: `attractionStrength` positive (0.02–0.04), `connectionRadius` 0.08–0.10
- **Disperse ParticleSet**: `attractionStrength` negative (-0.03 to -0.06), `connectionRadius` 0.12–0.16
- Different point sources for variety (Audio.PolarPitchRmsPoint vs Camera.PointVelocity)
- Different color sources (Palette.RandomLight vs Palette.RandomDark)

**Duplicate SoftCircle patterns:**
- **Mist**: Large radius (0.10–0.14), low alpha (0.08–0.12), targets fluid values
- **Accents**: Small radius (0.03–0.05), higher alpha (0.20–0.30), targets marks/accents layer

**Duplicate SandLine patterns:**
- **Sparse**: Low density (0.08–0.12), larger grains (PointRadius 16–24)
- **Dense**: Higher density (0.25–0.35), smaller grains (PointRadius 6–10)

**Smear configuration:**
- `MixNew`: 0.55–0.70 (moderate smearing)
- `Field1Multiplier`: 0.5–0.7
- `Field2Multiplier`: 0.3–0.5
- `AlphaMultiplier`: 0.995–0.997

**Latent layer alphas:**
- Base: 5–12% (higher than Rows 4–5 for visible complexity)
- ParticleSet layers: 8–12%
- Collage/geometry layers: 10–12%
- Soft marks layers: 5–8%

**Button colors (Row 6, saturation 0.40):**
`#FF9999`, `#FFBB99`, `#FFDD99`, `#FFFF99`, `#99FF99`, `#99FFDD`, `#99BBFF`, `#CC99FF`

## Per-row layer budgeting (rule of thumb)

- Keep a stable base (often `2400×2400` RGBA32F).
- Add at most one additional always-running heavy process (Fluid or Smear or ParticleField).
- Use overlays for complexity, mostly paused.
- Avoid multiple always-running `7200×7200` layers.

## Config validation tool

A config validator lives in `scripts/config-validator/` within the ofxMarkSynth addon. It validates synth config JSON files against the C++ source to ensure connection endpoints are valid.

**Location**: `ofxMarkSynth/scripts/config-validator/`

**Usage**:
```bash
# Validate a directory of configs
./run.sh /path/to/config/synth

# Validate a single config
./run.sh /path/to/config/synth/40-geometry-av-lines.json

# Strict mode (fails on warnings)
./run.sh /path/to/config/synth --strict
```

**What it validates**:
- JSON parses correctly
- All connection endpoints reference valid Mod types
- Source and sink names exist for the referenced Mod types

**How it stays in sync**:
- Parses ofxMarkSynth C++ sources to extract `sourceNameIdMap` and `sinkNameIdMap` entries
- Resolves `ofParameter` declarations to map `foo.getName()` references
- Runs in a Dockerized Python environment for stability

**When to run**:
- After creating or modifying synth configs
- Before committing config changes
- As part of CI/CD if integrated

## External references

See `performance-references.md` for a catalogue of tested performance config sets (CaldewRiver, Improvisation1, etc.) that can be used as reference material.
