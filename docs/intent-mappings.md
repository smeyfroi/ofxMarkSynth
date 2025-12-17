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

Mods with Intent support have an **AgencyFactor** parameter (default 1.0, range 0.0-1.0).

- **1.0**: Full Intent influence (default)
- **0.5**: Intent effect is halved
- **0.0**: Intent has no effect (Mod uses only its base parameters)

`AgencyFactor` is multiplied with the global agency value, controlling how much Intent affects that Mod. Configure via `"AgencyFactor": "0.5"` in synth config JSON.

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
| MultiplyAddMod | Yes | Yes | E, D, G |
| PathMod | Yes | Yes | D, G, C, S |
| PixelSnapshotMod | Yes | Yes | S, G |
| SomPaletteMod | Yes* | Yes | — (empty implementation) |
| VaryingValueMod | No | — | (commented out) |

## Process Mods - Detail

### ClusterMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| C | clusters | lin |

### MultiplyAddMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| E | Multiplier | exp |
| D*0.6 + G*0.4 | Adder | exp |

### PathMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| 1-D | MinVertexProximity | invExp |
| G | MaxVertexProximity | lin |
| D | MaxVertices | lin |
| C, S | Strategy | conditional (see below) |

Strategy selection:
- C > 0.6 AND S < 0.4 -> 2 (horizontals)
- S > 0.7 -> 3 (convex hull)
- S < 0.3 -> 0 (polypath)
- else -> 1 (bounds)

### PixelSnapshotMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| (1-S)*0.6 + G*0.4 | Size | lin |

---

## Sink Mods - Overview

| Mod | Intent | AgencyFactor | Primary Dimensions |
|-----|--------|--------------|-------------------|
| CollageMod | Yes | Yes | E, S, D, C, G |
| DividedAreaMod | Yes | Yes | C, G, E, D, S |
| FluidRadialImpulseMod | Yes | Yes | G, E, C |
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
| S or G | Strategy | 1 if S>0.55 or G>0.6, else 0 |
| 1-C | Outline | 1.0 if C<0.6, else 0.0 |

### DividedAreaMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| C | Angle | exp(0.0 -> 0.5) |
| G | PathWidth | exp(0.7) |
| E | MinorLineColour | energyToColor |
| D | MinorLineColour.alpha | lin(0.7 -> 1.0) |
| E | MajorLineColour | energyToColor * 0.7 |
| S | MajorLineColour.brightness | structureToBrightness * 0.8 |
| E*S | MajorLineColour.saturation | * 0.5 |
| D | MajorLineColour.alpha | exp(0.5)(0.0 -> 1.0) |
| C | MaxUnconstrainedLines | exp(2.0)(1 -> 9) |
| G | MajorLineWidth | lin |
| S | Strategy | conditional (see below) |

Strategy selection:
- S < 0.3 -> 0 (pairs)
- S 0.3-0.7 -> 1 (angles)
- S >= 0.7 -> 2 (radiating)

### FluidRadialImpulseMod
 
| Dimension | Parameter | Function |
|-----------|-----------|----------|
| G | Impulse Radius | lin |
| E (80%) + C (20%) | Impulse Strength | custom weighted |
| (none) | dt | manual (UI-controlled time step) |


### ParticleFieldMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| E | PointColour | energyToColor |
| S | PointColour.brightness | structureToBrightness * 0.5 |
| E*C | PointColour.saturation | direct |
| D | PointColour.alpha | lin(0.1 -> 0.5) |
| 1-G | minWeight | inv |
| C | maxWeight | lin |

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
| G | PointRadius | exp(3.0)(0.5 -> 16.0) |
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
| FluidMod | Yes | Yes | E, C, D, G |
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
| C | Vorticity | lin |
| 1-D | Value Dissipation | inv |
| 1-G | Velocity Dissipation | invExp |

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
| S | Strategy | conditional (see below) |
| S | GridLevels | lin + 1 (1-5) |
| 1-G | GridSize | lin(8 -> 64) |
| G | FoldPeriod | lin(4 -> 32) |

Strategy selection:
- S < 0.2 -> 0 (Off)
- S 0.2-0.4 -> 2 (Per-cell random offset)
- S 0.4-0.6 -> 4 (Per-cell rotation)
- S 0.6-0.8 -> 1 (Cell-quantized)
- S >= 0.8 -> 3 (Boundary teleport)

---

## Shader Mods - Overview

| Mod | Intent | AgencyFactor | Notes |
|-----|--------|--------------|-------|
| AddTextureMod | No | — | (commented out) |
| TranslateMod | No | — | (commented out) |

---

## Summary

**Active Intent implementations**: 18 Mods

**AgencyFactor support**: 16 Mods (all Mods we updated with per-Mod agency control)

**Mods without Intent**:
- AudioDataSourceMod, StaticTextSourceMod (source)
- VaryingValueMod (commented out)
- IntrospectorMod (sink)
- AddTextureMod, TranslateMod (commented out)

*Note: SomPaletteMod has AgencyFactor but empty applyIntent() body (ready for future implementation)*

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
**Last Updated**: December 5, 2025
