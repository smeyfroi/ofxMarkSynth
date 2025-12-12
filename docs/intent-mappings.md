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

All Mods with Intent support have an **AgencyFactor** parameter (default 1.0, range 0.0-1.0).

- **1.0**: Full Intent influence (default)
- **0.5**: Intent effect is halved
- **0.0**: Intent has no effect (Mod uses only its base parameters)

AgencyFactor is multiplied with the global agency value, allowing per-Mod control of how much the Intent system affects that Mod's behavior. Configure via `"AgencyFactor": "0.5"` in synth config JSON.

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
| VideoFlowSourceMod | No | — | — |

## Source Mods - Detail

### RandomFloatSourceMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| D | floatsPerUpdate | exp(0.5) |
| E | min, max | custom: E controls range width (20-100% of full range) |

### RandomHslColorMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| D | colorsPerUpdate | exp(2.0) |
| E | hueCenter | lerp(0.6 -> 0.08) cool to warm |
| C | hueWidth | lerp(0.08 -> 1.0) |
| E | minSaturation | lin(0.2 -> 0.8) |
| E | maxSaturation | lin(0.6 -> 1.0) |
| S | minBrightness | inv(0.4 -> 0.1) |
| S | maxBrightness | lin(0.6 -> 1.0) |
| D | minAlpha | lin(0.2 -> 0.8) |
| D | maxAlpha | lin(0.6 -> 1.0) |

### RandomVecSourceMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| D | vecsPerUpdate | exp(2.0) |

### TextSourceMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| C | randomness | direct (0=sequential, 1=random) |

### TimerSourceMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| 1-E | interval | exp(0.5) — higher energy = shorter interval |

---

## Process Mods - Overview

| Mod | Intent | AgencyFactor | Primary Dimensions |
|-----|--------|--------------|-------------------|
| ClusterMod | Yes | — | C |
| MultiplyAddMod | Yes | — | E, D, G |
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
| E | multiplier | exp |
| D*0.6 + G*0.4 | adder | exp |

### PathMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| 1-D | minVertexProximity | invExp |
| G | maxVertexProximity | lin |
| D | maxVertices | lin |
| C, S | strategy | conditional (see below) |

Strategy selection:
- C > 0.6 AND S < 0.4 -> 2 (horizontals)
- S > 0.7 -> 3 (convex hull)
- S < 0.3 -> 0 (polypath)
- else -> 1 (bounds)

### PixelSnapshotMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| (1-S)*0.6 + G*0.4 | size | lin |

---

## Sink Mods - Overview

| Mod | Intent | AgencyFactor | Primary Dimensions |
|-----|--------|--------------|-------------------|
| CollageMod | Yes | Yes | E, S, D, C, G |
| DividedAreaMod | Yes | Yes | C, G, E, D, S |
| FluidRadialImpulseMod | Yes | Yes | G, E, C |
| IntrospectorMod | No | — | — |
| ParticleFieldMod | Yes | Yes | E, S, C, D, G |
| ParticleSetMod | Yes | — | E, C, S, D, G |
| SandLineMod | Yes | Yes | E, G, S, C, D |
| SoftCircleMod | Yes | — | E, D |
| TextMod | Yes | — | G, E, D, C |

## Sink Mods - Detail

### CollageMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| E | color | energyToColor, mixed |
| S | color.brightness | structureToBrightness (25% mix) |
| D | color.alpha | densityToAlpha(0.3 -> 1.0) |
| E | saturation (part 1) | lin(0.8 -> 2.2) |
| C | saturation (part 2) | exp(0.9 -> 2.8) |
| 1-S | saturation (part 3) | inv(0.8 -> 1.6) |
| S or G | strategy | 1 if S>0.55 or G>0.6, else 0 |
| 1-C | outline | 1.0 if C<0.6, else 0.0 |

### DividedAreaMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| C | angle | exp(0.0 -> 0.5) |
| G | pathWidth | exp(0.7) |
| E | minorLineColor | energyToColor |
| D | minorLineColor.alpha | lin(0.7 -> 1.0) |
| E | majorLineColor | energyToColor * 0.7 |
| S | majorLineColor.brightness | structureToBrightness * 0.8 |
| E*S | majorLineColor.saturation | * 0.5 |
| D | majorLineColor.alpha | exp(0.5)(0.0 -> 1.0) |
| C | maxUnconstrainedLines | exp(2.0)(1 -> 9) |
| G | majorLineWidth | lin |
| S | strategy | conditional (see below) |

Strategy selection:
- S < 0.3 -> 0 (pairs)
- S 0.3-0.7 -> 1 (angles)
- S >= 0.7 -> 2 (radiating)

### FluidRadialImpulseMod
 
| Dimension | Parameter | Function |
|-----------|-----------|----------|
| G | impulseRadius | lin |
| E (80%) + C (20%) | impulseStrength | custom weighted |
| (none) | dt | manual (UI-controlled time step) |


### ParticleFieldMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| E | color | energyToColor |
| S | color.brightness | structureToBrightness * 0.5 |
| E*C | color.saturation | direct |
| D | color.alpha | lin(0.1 -> 0.5) |
| 1-G | minWeight | inv |
| C | maxWeight | lin |

### ParticleSetMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| E*C | spin | lin(-0.05 -> 0.05) |
| E | color | energyToColor |
| S | color.brightness | structureToBrightness |
| E*C*(1-S) | color.saturation | direct |
| D | color.alpha | lin(0.0 -> 1.0) |
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
| E*G | density | exp |
| G | pointRadius | exp(3.0)(0.5 -> 16.0) |
| E | color | energyToColor |
| S | color.brightness | structureToBrightness |
| E*(1-S) | color.saturation | direct |
| 1-S | stdDevAlong | inv |
| C | stdDevPerpendicular | lin |

### SoftCircleMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| E | radius | exp |
| D | colorMultiplier | lin |
| D | alphaMultiplier | lin |

### TextMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| G | fontSize | exp |
| E | color | energyToColor |
| D | color.alpha | lin(0.5 -> 1.0) |
| D | alpha | lin |
| 1-G | DrawDurationSec | exp |
| D | AlphaFactor | exp |
| C | position | jitter (max 15% when C>0.1) |

---

## Layer Mods - Overview

| Mod | Intent | AgencyFactor | Primary Dimensions |
|-----|--------|--------------|-------------------|
| FadeMod | Yes | Yes | D, G |
| FluidMod | Yes | — | E, C, D, G |
| SmearMod | Yes | Yes | E, D, C, G, S |

## Layer Mods - Detail

### FadeMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| D*0.8 + G*0.2 | alphaMultiplier | exp |

### FluidMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| E | dt | exp |
| C | vorticity | lin |
| 1-D | valueDissipation | inv |
| 1-G | velocityDissipation | invExp |

### SmearMod

| Dimension | Parameter | Function |
|-----------|-----------|----------|
| E | mixNew | exp |
| D | alphaMultiplier | exp |
| E | field1Multiplier | exp(2.0) |
| C | field2Multiplier | exp(3.0) |
| C | jumpAmount | exp(2.0) |
| G | borderWidth | lin |
| D | ghostBlend | lin |
| S | strategy | conditional (see below) |
| S | gridLevels | lin + 1 (1-5) |
| 1-G | gridSize | lin(8 -> 64) |
| G | foldPeriod | lin(4 -> 32) |

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
- AudioDataSourceMod, StaticTextSourceMod, VideoFlowSourceMod (source)
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
