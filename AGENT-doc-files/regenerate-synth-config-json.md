# Regenerating docs/synth-config-reference.json

This document describes how to regenerate the `docs/synth-config-reference.json` file, which contains all Mod types and their default parameter values for use as a reference when creating Synth configurations.

## Purpose

The `synth-config-reference.json` file serves as a comprehensive reference of all available Mod types with their default parameter values. Users can copy/paste from this file when creating new Synth configurations.

## When to Regenerate

Regenerate this file when:
- A new Mod type is added to `ModFactory`
- Parameters are added/removed/renamed in any Mod
- Default parameter values change
- External dependency parameters change (ofxParticleSet, ofxPointClusters, etc.)

## Source Files to Check

### ModFactory Registration
- `src/util/ModFactory.cpp` - Lists all registered Mod types

### Mod Parameter Definitions

#### Source Mods (`src/sourceMods/`)
- `AudioDataSourceMod.hpp` - Audio analysis parameters
- `StaticTextSourceMod.hpp` - Static text emission
- `TextSourceMod.hpp` - File-based text source
- `TimerSourceMod.hpp` - Timer/interval source
- `RandomFloatSourceMod.hpp` - Random float generation
- `RandomHslColorMod.hpp` - Random HSL color generation
- `RandomVecSourceMod.hpp` - Random vector generation
- `VideoFlowSourceMod.hpp` - Video/camera optical flow (+ ofxMotionFromVideo params)

#### Process Mods (`src/processMods/`)
- `ClusterMod.hpp` - Point clustering (+ ofxPointClusters params)
- `MultiplyAddMod.hpp` - Multiply/add transform
- `PathMod.hpp` - Path generation from points
- `PixelSnapshotMod.hpp` - FBO region capture
- `SomPaletteMod.hpp` - Self-organizing map palette
- `VaryingValueMod.hpp` - Value interpolation

#### Sink Mods (`src/sinkMods/`)
- `CollageMod.hpp` - Collage rendering
- `DividedAreaMod.hpp` - Divided area line drawing
- `FluidRadialImpulseMod.hpp` - Fluid impulse injection
- `IntrospectorMod.hpp` - Debug visualization
- `ParticleFieldMod.hpp` - GPU particle field (+ ofxParticleField params)
- `ParticleSetMod.hpp` - CPU particle system (+ ofxParticleSet params)
- `SandLineMod.hpp` - Sand/stipple line rendering
- `SoftCircleMod.hpp` - Soft circle rendering
- `TextMod.hpp` - Text rendering

#### Layer Mods (`src/layerMods/`)
- `FadeMod.hpp` - Alpha fade effect
- `FluidMod.hpp` - Fluid simulation (+ ofxRenderer FluidSimulation params)
- `SmearMod.hpp` - Smear/displacement effect

### External Dependency Parameters

These addons expose parameters via `getParameterGroup()` that are flattened into Mod parameter groups using `addFlattenedParameterGroup()`:

| Addon | Header Location | Used By |
|-------|-----------------|---------|
| ofxParticleSet | `../ofxParticleSet/src/ofxParticleSet.h` | ParticleSetMod |
| ofxPointClusters | `../ofxPointClusters/src/ofxPointClusters.h` | ClusterMod |
| ofxRenderer | `../ofxRenderer/src/fluid/FluidSimulation.h` | FluidMod |
| ofxMotionFromVideo | `../ofxMotionFromVideo/src/ofxMotionFromVideo.h` | VideoFlowSourceMod |
| ofxParticleField | `../ofxParticleField/src/ParticleField.h` | ParticleFieldMod |

## JSON Structure

```json
{
  "version": "1.0",
  "description": "Reference of all Mod types with default parameter values",
  "mods": {
    "ExampleAudioSource": {
      "type": "AudioDataSource",
      "config": {
        "MinPitch": "50",
        "MaxPitch": "800"
      }
    }
  }
}
```

### Parameter Value Formats

Values are strings matching `ofParameter::toString()` output:
- **float**: `"0.5"`
- **int**: `"10"`
- **bool**: `"1"` (true) or `"0"` (false)
- **string**: `"text value"`
- **ofFloatColor**: `"0.5, 0.5, 0.5, 1"` (r, g, b, a)
- **ofColor**: `"255, 255, 0, 255"` (r, g, b, a)
- **glm::vec2**: `"0.5, 0.5"` (x, y)

## Checklist for Regeneration

### Source Mods (8)
- [ ] AudioDataSource
- [ ] StaticTextSource
- [ ] TextSource
- [ ] TimerSource
- [ ] RandomFloatSource
- [ ] RandomHslColor
- [ ] RandomVecSource
- [ ] VideoFlowSource

### Process Mods (7)
- [ ] Cluster
- [ ] Fluid
- [ ] MultiplyAdd
- [ ] Path
- [ ] PixelSnapshot
- [ ] Smear
- [ ] SomPalette

### Sink Mods (9)
- [ ] Collage
- [ ] DividedArea
- [ ] FluidRadialImpulse
- [ ] Introspector
- [ ] ParticleField
- [ ] ParticleSet
- [ ] SandLine
- [ ] SoftCircle
- [ ] Text

### Layer Mods (1)
- [ ] Fade

## Process

1. Read `ModFactory.cpp` to get the current list of registered types
2. For each Mod type, read its `.hpp` file to extract `ofParameter<T>` declarations
3. Note the parameter name (first constructor arg), default value (second arg), and type
4. For Mods using `addFlattenedParameterGroup()`, also read the external dependency headers
5. Format values as strings according to `ofParameter::toString()` conventions
6. Update `docs/synth-config-reference.json` with all parameters

## Notes

- Some Mods require external resources (fonts, audio files, video) and cannot be instantiated without them. Their parameters are still documented from source.
- The `config` section in the JSON matches what `SynthConfigSerializer` expects when loading configurations.
- Parameter names must match exactly (case-sensitive) for serialization to work.
