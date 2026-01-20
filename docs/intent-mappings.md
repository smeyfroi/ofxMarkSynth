# Intent Mappings Reference

This document details how each Mod responds to the Intent system.

See also: [mods.md](mods.md) for full Mod documentation.

---

## Intent Dimensions

| Dimension | Code | Purpose |
|-----------|------|---------|
| Energy | E | Speed, motion, intensity, magnitude |
| Density | D | Quantity, opacity, detail level |
| Structure | S | Organization, brightness, pattern regularity |
| Chaos | C | Randomness, variance, noise |
| Granularity | G | Scale, resolution, feature size |

All dimensions are normalized to 0.0-1.0.

---

## Example Intents

- Intent::createPreset("Calm", 0.2f, 0.3f, 0.7f, 0.1f, 0.1f),
- Intent::createPreset("Energetic", 0.9f, 0.7f, 0.4f, 0.5f, 0.5f),
- Intent::createPreset("Chaotic", 0.6f, 0.2f, 0.1f, 0.95f, 0.4f),
- Intent::createPreset("Dense", 0.5f, 0.95f, 0.6f, 0.3f, 0.5f),
- Intent::createPreset("Structured", 0.4f, 0.5f, 0.95f, 0.2f, 0.4f),
- Intent::createPreset("Minimal", 0.1f, 0.1f, 0.8f, 0.05f, 0.1f),
- Intent::createPreset("Maximum", 0.95f, 0.95f, 0.5f, 0.8f, 0.95f),

---

## Mapping Functions

| Function | Behavior |
|----------|----------|
| `lin` | Linear: `lerp(min, max, v)` |
| `exp(n)` | Exponential: `lerp(min, max, pow(v, n))` — more responsive at low values |
| `inv` | Inverse linear: `lerp(max, min, v)` |
| `invExp(n)` | Inverse exponential: `lerp(min, max, pow(1-v, n))` |

---

## AgencyFactor

Many Mods have an **AgencyFactor** parameter (default 1.0, range 0.0–1.0).

`AgencyFactor` scales a Mod’s effective **Synth Agency** for auto-wired parameters.

- **1.0**: Full auto takeover potential (uses the global Synth Agency)
- **0.5**: Half auto takeover potential
- **0.0**: No auto takeover (connected auto values still arrive, but the Mod treats agency as 0)

In code, Mods typically compute:

- `effectiveAgency = SynthAgency * AgencyFactor`

and pass that value into `ParamController::updateAuto(...)`.

Note: Intent influence is controlled by the Synth intent strength/activations and each Mod’s `applyIntent()` mapping. Most Mods do **not** currently scale intent strength by `AgencyFactor`.

---

## Node Editor Visual Feedback

The Node Editor provides visual cues to indicate which Mods are actively responding to agency:

### Agency Meter
Each Mod node displays a small progress bar in its title bar:
- **Active (colored bar)**: Shown when the Mod has `agency > 0` AND has received automatic values through connections. The bar level indicates the effective agency value.
- **Inactive (subtle grey)**: Shown when the Mod is not responding to agency (either agency is 0, or no parameters have received automatic values).

### Title Bar Color
Mod nodes change their title bar color based on agency state:
- **Default blue**: Standard Mod nodes not actively responding to agency
- **Purple-blue tint**: Mods actively responding to agency (color shifts towards red/purple to match the red agency indicator on MIDI controllers)
- **Green**: Highlighted Mods (e.g., after loading a snapshot)

This visual distinction helps identify at a glance which parts of the synth graph are under autonomous Synth control versus manual control.

---

## Source Mods - Overview

| Mod | Intent | AgencyFactor | Primary Dimensions |
|-----|--------|--------------|-------------------|
| AudioDataSourceMod | No | — | — |
| RandomFloatSourceMod | Yes | Yes | D, E |
| RandomHslColorMod | Yes | Yes | D, E, C, S |
| RandomVecSourceMod | Yes | Yes | D |
| StaticTextSourceMod | No | — | — |
| TextSourceMod | Yes | Yes | C |
| TimerSourceMod | Yes | Yes | E |
| VideoFlowSourceMod | Yes | Yes | D |

## Source Mods - Detail

### RandomFloatSourceMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| D | CreatedPerUpdate | exp(0.5) |
| E | Min, Max | custom: E controls range width (20-100% of full range) |

### RandomHslColorMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| D | CreatedPerUpdate | exp(2.0) |
| E | HueCenter | lerp(0.6 -> 0.08) cool to warm |
| C | HueWidth | lerp(0.08 -> 1.0) |
| E | MinSaturation | lin(0.2 -> 0.8) |
| E | MaxSaturation | lin(0.6 -> 1.0) |
| S | MinBrightness | inv(0.4 -> 0.1) |
| S | MaxBrightness | lin(0.6 -> 1.0) |
| D | MinAlpha | lin(0.2 -> 0.8) |
| D | MaxAlpha | lin(0.6 -> 1.0) |

### RandomVecSourceMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| D | CreatedPerUpdate | exp(2.0) |

### TextSourceMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| C | Randomness | direct (0=sequential, 1=random) |

### TimerSourceMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| 1-E | Interval | exp(0.5) — higher energy = shorter interval |

### VideoFlowSourceMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| D | PointSamplesPerUpdate | exp(0.5) |

---

## Process Mods - Overview

| Mod | Intent | AgencyFactor | Primary Dimensions |
|-----|--------|--------------|-------------------|
| ClusterMod | Yes | Yes | C |
| MultiplyAddMod | No (by design) | Yes | — |
| VectorMagnitudeMod | No | No | — |
| PathMod | Yes (partial) | Yes | D, G |
| PixelSnapshotMod | Yes | Yes | S, G |
| SomPaletteMod | No (by design) | Yes | — |
| VaryingValueMod | No | — | (commented out) |

## Process Mods - Detail

### ClusterMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| C | clusters | lin |

### MultiplyAddMod

No Intent mapping (by design).

Rationale:
- `MultiplyAddMod` is used as a low-level utility for scaling/offsetting envelopes.
- Driving it from high-level Intent tends to create confusing global side effects (it changes the meaning of upstream signals).
- If you want Intent to shape a derived envelope, prefer mapping Intent directly on the target Mod, or add a dedicated "IntentMap" mod with performer-visible semantics.

### VectorMagnitudeMod

No Intent mapping (currently). This is a utility mod for deriving scalar envelopes from vector streams.

### PathMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| G | ClusterRadius | exp — higher granularity = larger cluster radius (bigger shapes) |
| D | MaxVertices | exp |

**Not Intent-driven (by design):** `Strategy`

Rationale:
- Strategy switching is a discontinuous jump (it can read like a glitch or mode change).
- In performance it’s often preferable to switch `Strategy` explicitly (via config choice, triggers, or dedicated controls), rather than having it drift with Intent.

Note: there is commented-out experimental code for Strategy selection based on Chaos/Structure in `src/processMods/PathMod.cpp:275`.

**Parameter semantics:**
- **ClusterRadius**: Maximum distance from the newest point for inclusion in the cluster (0.01-1.0 normalized). All points within this radius of the newest point are collected, then shaped by the Strategy (polypath, bounds, convex hull, etc.).

(Intentionally not used for Intent in current design.)

The codebase contains a commented-out experimental Strategy mapping based on Chaos/Structure in `src/processMods/PathMod.cpp:275`:
- C > 0.6 AND S < 0.4 -> 2 (horizontals)
- S > 0.7 -> 3 (convex hull)
- S < 0.3 -> 0 (polypath)
- else -> 1 (bounds)

### PixelSnapshotMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| (1-S)*0.6 + G*0.4 | Size | lin |

### SomPaletteMod

No Intent mapping (by design).

Rationale:
- The palette is treated as an external “world state” driven by samples, palette switching, and/or explicit performer controls.
- Keeping it out of Intent helps preserve stable color identity within a config.

---

## Sink Mods - Overview

| Mod | Intent | AgencyFactor | Primary Dimensions |
|-----|--------|--------------|-------------------|
| CollageMod | Yes | Yes | E, S, D, C, G |
| DividedAreaMod | Yes | Yes | C, G, E, D, S |
| FluidRadialImpulseMod | Yes | Yes | G, E, C, S |
| IntrospectorMod | No | — | — |
| ParticleFieldMod | Yes | Yes | E, S, C, D, G |
| ParticleSetMod | Yes | Yes | E, C, S, D, G |
| SandLineMod | Yes | Yes | E, G, S, C, D |
| SoftCircleMod | Yes | — | E, D |
| TextMod | Yes | — | G, E, D, C |

## Sink Mods - Detail

### CollageMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| E | Colour | energyToColor, mixed |
| S | Colour.brightness | structureToBrightness (25% mix) |
| D | Colour.alpha | densityToAlpha(0.3 -> 1.0) |
| E | Saturation (part 1) | lin(0.8 -> 2.2) |
| C | Saturation (part 2) | exp(0.9 -> 2.8) |
| 1-S | Saturation (part 3) | inv(0.8 -> 1.6) |
| S * (1-C) | OutlineAlphaFactor | S lin(0.2 -> 1.0) * inv(C)(0.5 -> 1.0) |
| E * (1-G) | OutlineWidth | E lin(8 -> 24) * inv(G)(0.5 -> 1.2) |
| 1-S | OutlineColour.brightness | invExp(0.3 -> 1.0) for contrast |
| E | OutlineColour.warmth | lin(0.0 -> 0.15) subtle warm shift |

**Non-intent compositing controls (manual):**
- `BlendMode` (0..4): Selects blend mode for Collage drawing (0=ALPHA, 1=SCREEN, 2=ADD, 3=MULTIPLY, 4=SUBTRACT).
- `Opacity` (0.0..1.0): Multiplies `Colour.alpha` before drawing (useful to tame SCREEN/ADD blowout).
- `MinDrawInterval` (seconds): Rate limits drawing; when throttled, pending `Path`/`SnapshotTexture` are kept until the next eligible draw.

### DividedAreaMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| C | Angle | exp(0.0 -> 0.5) |
| G | PathWidth | exp(1.0) |
| E | MinorLineColour | energyToColor |
| D | MinorLineColour.alpha | lin(0.7 -> 1.0) |
| C | MaxUnconstrainedLines | exp(2.0)(1 -> 9) |
| G | MajorLineWidth | lin |
| S | Strategy | conditional (see below) |
| 1-S | unconstrainedSmoothness | inv.lin(0.3 -> 0.9) |

**unconstrainedSmoothness**: Controls how smoothly major (unconstrained) lines track their input points. High Structure = high smoothness (stable, graceful motion). Low Structure = low smoothness (responsive, direct tracking). Uses spring-damper physics with hysteresis for natural motion.

Strategy selection:
- S < 0.3 -> 0 (pairs)
- S 0.3-0.7 -> 1 (angles)
- S >= 0.7 -> 2 (radiating)

### FluidRadialImpulseMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| G * (1 - 0.4*S) | Impulse Radius | exp(G) then attenuate by Structure |
| (E*0.8 + C*0.2) * (1 - 0.7*S) | Impulse Strength | exp(4.0)(0.0 -> 0.6×max) weighted, then attenuate by Structure |
| C * (1 - 0.7*S) | SwirlStrength | exp(2.0) — increases with Chaos, reduced by Structure |
| C * (1 - 0.7*S) | SwirlVelocity | exp(2.0) — increases with Chaos, reduced by Structure |
| (none) | dt | manual (UI-controlled time step) |

**Manual-only (not Intent-driven)**: `VelocityScale`.


### ParticleFieldMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| E | PointColour | energyToColor |
| S | PointColour.brightness | structureToBrightness * 0.5 |
| E*C | PointColour.saturation | direct |
| D | PointColour.alpha | lin(0.1 -> 0.5) |
| 1-G | minWeight | inv |
| C | maxWeight | lin |
| 1-G | velocityDamping | inv — high G = low damping (more motion) |
| E | forceMultiplier | exp — energy = force intensity |
| E | maxVelocity | lin — energy = speed |
| G | particleSize | exp — granularity = feature size |
| C | jitterStrength | lin — chaos = randomness |
| S | jitterSmoothing | lin — structure = smoothness |
| E*C | speedThreshold | lin — activity threshold |
| E | field1Multiplier | exp(2.0) — energy controls primary field |
| C | field2Multiplier | exp(3.0) — chaos controls secondary field |

### ParticleSetMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| E*C | Spin | lin(-0.05 -> 0.05) |
| E | Colour | energyToColor |
| S | Colour.brightness | structureToBrightness |
| E*C*(1-S) | Colour.saturation | direct |
| D | Colour.alpha | lin(0.0 -> 1.0) |
| E | timeStep | exp |
| 1-G | velocityDamping | inv |
| S | attractionStrength | lin |
| 1-D | attractionRadius | inv |
| E | forceScale | lin |
| D | connectionRadius | lin |
| E | colourMultiplier | lin |
| C | maxSpeed | lin |

### SandLineMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| E*G | Density | exp |
| G | PointRadius | exp(3.0)(1.0 -> 16.0) |
| E | Colour | energyToColor |
| S | Colour.brightness | structureToBrightness |
| E*(1-S) | Colour.saturation | direct |
| 1-S | StdDevAlong | inv |
| C | StdDevPerpendicular | lin |

### SoftCircleMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| E | Radius | exp |
| D | ColourMultiplier | exp(4.0) [1.0, 1.4] — flat at low D, accelerates toward 1.4 |
| D | AlphaMultiplier | lin |

### TextMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| G | FontSize | exp |
| E | Colour | energyToColor |
| D | Colour.alpha | lin(0.5 -> 1.0) |
| D | Alpha | lin |
| 1-G | DrawDurationSec | exp |
| D | AlphaFactor | exp |
| C | Position | jitter (max 15% when C>0.1) |

---

## Layer Mods - Overview

| Mod | Intent | AgencyFactor | Primary Dimensions |
|-----|--------|--------------|-------------------|
| FadeMod | Yes | Yes | D, G |
| FluidMod | Yes | Yes | E, C, D, G, S |
| SmearMod | Yes | Yes | E, D, C, G, S |

## Layer Mods - Detail

### FadeMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| D*0.8 + G*0.2 | Alpha | exp |

### FluidMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| E | dt | exp |
| C * (1 - 0.75*S) | Vorticity | lin — Structure attenuates Chaos for laminar flow |
| 1-D | Value Dissipation | inv |
| 1-G | Velocity Dissipation | invExp |

**Manual-only (not Intent-driven)**: `Boundary Mode`, `Value Spread`, `Velocity Spread`, `Value Max`, all `Temperature` params (including `TempEnabled`), `TempImpulseRadius`, `TempImpulseDelta`, all `Obstacles` params (`ObstaclesEnabled`, `Obstacle Threshold`, `Obstacle Invert`), and `Buoyancy` params.

### SmearMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| E | MixNew | exp |
| D | AlphaMultiplier | exp |
| E | Field1Multiplier | exp(2.0) |
| C | Field2Multiplier | exp(3.0) |
| C | JumpAmount | exp(2.0) |
| G | BorderWidth | lin |
| D | GhostBlend | lin |
| S | GridLevels | lin + 1 (1-5) |
| 1-G | GridSize | lin(8 -> 64) |
| G | FoldPeriod | lin(4 -> 32) |

*Note: Strategy, Field1PreScaleExp, Field2PreScaleExp, Field1Bias, and Field2Bias are manual-only parameters (not controlled by Intent). The PreScaleExp parameters are log10 exponents (-3.0 to 2.0) that normalize incoming field magnitudes before the Intent-controlled multipliers.*

---

## Shader Mods - Overview

| Mod | Intent | AgencyFactor | Notes |
|-----|--------|--------------|-------|
| AddTextureMod | No | — | (commented out) |
| TranslateMod | No | — | (commented out) |

---

## Summary

**Mods with `applyIntent()`**: 23 total (includes `Synth` + `MemoryBankController`).

**Performer-visible Intent mappings**:
- Most mark-making Mods implement meaningful mappings.
- Intent is intentionally excluded from:
  - `MultiplyAddMod` (utility scaling/offset)
  - `SomPaletteMod` (palette is treated as external world state)
  - `PathMod` Strategy switching (discontinuous; keep explicit)

**AgencyFactor support**:
- Most Mods that can receive automation also support per-Mod `AgencyFactor`.

**Mods without Intent** (non-exhaustive):
- `AudioDataSourceMod`, `StaticTextSourceMod` (source)
- `VectorMagnitudeMod` (process utility)
- `IntrospectorMod` (sink)
- `VaryingValueMod`, `AddTextureMod`, `TranslateMod` (commented out)

**Dimension usage frequency**:
| Dimension | Mods Using |
|-----------|------------|
| Energy | 16 |
| Density | 14 |
| Chaos | 11 |
| Structure | 11 |
| Granularity | 11 |

---

**Document Version**: 1.0
**Last Updated**: January 19, 2026
