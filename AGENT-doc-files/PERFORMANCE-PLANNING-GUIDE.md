# Performance Planning Guide for ofxMarkSynth

## Overview

This guide describes how to create performance plans and cue sheets for live audio-visual performances using ofxMarkSynth as an improvisation "drawing instrument" in concert with improvising musicians.

**Context**: Contemporary art performance with acoustic improvisation  
**Role of MarkSynth**: Visual synthesizer that responds to audio and movement, creating accumulating marks that persist as temporal artifacts  
**Core Concept**: The visual layer creates "performance fingerprints" - unique traces left by musicians as they improvise through space and time

---

## Document Structure

A complete performance requires three types of documents:

1. **Performance Plan** (`performance-[name].md`) - Conceptual framework, narrative arc, and detailed section descriptions
2. **Configuration Files** (`performance-configs/*.json`) - Technical specifications for each section
3. **Cue Sheet** (`PERFORMANCE-CUE-SHEET.md`) - Condensed timing and action reference for live performance

---

## Part 1: Creating the Performance Plan

### 1.1 Establish the Conceptual Framework

Begin with a **central metaphor** that guides the entire work. This metaphor should:
- Provide a narrative arc with clear progression
- Suggest visual transformations over time
- Connect naturally to musical structure

**Example metaphors**:
- River journey (source → tributaries → cataract → meander → delta)
- Life cycle (seed → growth → bloom → decay → return)
- Breath (inhale → hold → exhale → pause)
- Memory (clear → layered → fragmented → dissolved)

Document the metaphor with:
```markdown
## Concept

A [duration]-movement audio-visual performance exploring [theme].
The visual synthesizer responds to both audio (fixed microphone) and 
movement (fixed camera), creating accumulating marks that persist as 
temporal artifacts.

**Metaphor**: [Your central metaphor]
**Context**: Contemporary art performance with acoustic improvisation
**Duration**: ~[X] minutes
**Output**: [Number] still images captured at the end of each movement
```

### 1.2 Define the Narrative Arc

Plan the progression across multiple dimensions:

#### Granularity Progression
How mark-making evolves from simple to complex (or vice versa):
- Points (atomic) → Lines (connecting) → Areas (continuous) → Flows (smeared) → Atmosphere (dissolved)

#### Energy Arc
The intensity curve of the performance:
- Identify the climax (typically 2/3 through)
- Plan build-ups and releases
- Consider how energy affects visual density

#### Color Evolution
How palette changes throughout:
- Starting palette (often minimal/monochromatic)
- Peak complexity (full palette, learned colors)
- Resolution (return to simplicity or new state)

Document as:
```markdown
## Narrative Arc & Visual Evolution

### Granularity Progression
- **Movement 1**: [granularity description]
- **Movement 2**: [granularity description]
...

### Energy Arc
- **Movement 1**: [energy description]
- **Movement 2**: [energy description]
...

### Color Evolution
- **Movement 1**: [color description]
- **Movement 2**: [color description]
...
```

### 1.3 Define Technical Setup

Document the fixed input configuration:

```markdown
## Technical Setup

### Fixed Inputs
- **Microphone**: [Placement strategy, type, what it captures]
- **Camera**: [Placement strategy, what movement it captures]
- **Performers**: [Number, mobility expectations]

### Fingerprint Sources
1. **Audio**: Pitch, loudness, timbre, spectral features
2. **Movement**: Position, velocity, direction via optical flow
3. **Spatial**: Where performers are in relation to fixed mic/camera
4. **Temporal**: What persists vs. what fades

### Capture Requirements
- High-resolution frame capture at end of each movement
- Timestamp for each capture point
```

### 1.4 Design Each Movement

For each movement, document:

#### Movement Header
```markdown
## Movement [N]: "[Title]" ([Duration])
**Form**: [Musical form - Binary, Ternary, Rondo, Passacaglia, etc.]
**Performers**: [Number and mobility]
**Visual Theme**: [What this movement contributes visually]
```

#### Sections Within Movement
Each section needs:
```markdown
### Section [Letter] ([Duration]) - [Section Title]
**Config**: `movement[N]-[name]-[section].json`

**Visual Elements**:
- [Mod type] ([input source]) - [visual effect]
- [Layer interactions]
- [Fade rate and persistence]

**Performer Action**: [What musicians do]

[If transition needed]:
**[Fade/Transition type]**
```

#### Capture Points
```markdown
**CAPTURE**: [Timestamp] - "[Fingerprint Name]"
*[Description of expected visual result]*
```

### 1.5 Document Intent System Mapping

Explain how live control dimensions map to visual parameters:

```markdown
## Intent System Mapping

### Live Control Dimensions
1. **[Dimension Name]** → [What parameters it affects]
2. **[Dimension Name]** → [What parameters it affects]
...

### Automatic Contributions
- **[Input source]** → [Which dimension]
- **[Input source]** → [Which dimension]
```

---

## Part 2: Creating Configuration Files

### 2.1 Configuration File Structure

Each JSON configuration file follows this structure:

```json
{
  "version": "1.0",
  "description": "Brief description of what happens visually",
  
  "synth": {
    "agency": 0.5,
    "backgroundColor": "R, G, B, A",
    "_comment": "Optional explanatory comments"
  },
  
  "drawingLayers": {
    "layerName": {
      "size": [2048, 2048],
      "internalFormat": "GL_RGBA32F",
      "wrap": "GL_CLAMP_TO_EDGE",
      "clearOnUpdate": false,
      "blendMode": "OF_BLENDMODE_ALPHA",
      "isDrawn": true,
      "_comment": "Purpose of this layer"
    }
  },
  
  "mods": {
    "ModInstanceName": {
      "type": "ModType",
      "_comment": "What this mod does in this context",
      "config": {
        "Parameter": "value"
      },
      "layers": {
        "default": ["layerName"]
      }
    }
  },
  
  "connections": [
    "SourceMod.Output -> SinkMod.Input"
  ]
}
```

### 2.2 Layer Configuration

#### Single Layer (Simple)
For minimal configurations with one accumulating surface:
```json
"drawingLayers": {
  "main": {
    "size": [2048, 2048],
    "internalFormat": "GL_RGBA32F",
    "wrap": "GL_CLAMP_TO_EDGE",
    "clearOnUpdate": false,
    "blendMode": "OF_BLENDMODE_ALPHA",
    "isDrawn": true
  }
}
```

#### Multiple Layers (Complex)
For layered compositions with different behaviors:
```json
"drawingLayers": {
  "fluid": { ... },      // Fluid simulation
  "particles": { ... },  // Particle systems
  "geometry": { ... },   // Geometric divisions
  "highlights": {        // Additive layer
    "blendMode": "OF_BLENDMODE_ADD",
    ...
  },
  "velocities": {        // Non-visible data layer
    "size": [512, 512],
    "internalFormat": "GL_RG32F",
    "clearOnUpdate": true,
    "isDrawn": false
  }
}
```

### 2.3 Available Mod Types

#### Source Mods (Input)
| Type | Purpose | Key Parameters |
|------|---------|----------------|
| `AudioDataSource` | Microphone/audio file input | MinPitch, MaxPitch, MinRms, MaxRms |
| `VideoFlowSource` | Camera/video optical flow | Flow thresholds |
| `TimerSource` | Timed triggers | Interval, Enabled, OneShot |
| `RandomFloatSource` | Random values | Min, Max, CreatedPerUpdate |
| `RandomHslColor` | Random colors | HueCenter, HueWidth, Saturation/Lightness ranges |
| `RandomVecSource` | Random vectors | CreatedPerUpdate |
| `TextSource` | Text file input | TextFilename, ParseWords, Loop |
| `StaticTextSource` | Fixed text | Text, EmitOnce, Delay |

#### Process Mods (Transform)
| Type | Purpose | Key Parameters |
|------|---------|----------------|
| `Cluster` | Group points into clusters | Clusters, Max Source Points |
| `Fluid` | Fluid simulation | dt, Vorticity, Dissipation |
| `Path` | Create paths from points | Strategy, MaxVertices |
| `PixelSnapshot` | Capture layer regions | SnapshotsPerUpdate, Size |
| `Smear` | Distort/smear layer content | Strategy (0-9), MixNew, Field multipliers |
| `SomPalette` | Learn colors from input | Iterations |
| `MultiplyAdd` | Transform values | Multiplier, Adder |

#### Sink Mods (Output)
| Type | Purpose | Key Parameters |
|------|---------|----------------|
| `SoftCircle` | Soft-edged circles | Radius, Colour, Softness |
| `SandLine` | Stippled/sandy lines | Density, StdDev parameters |
| `ParticleSet` | Connected particle system | maxParticles, connectionRadius, strategy |
| `ParticleField` | Grid-based particle field | ln2ParticleCount, velocityDamping |
| `DividedArea` | Geometric line divisions | Strategy, LineWidths, LineColours |
| `Collage` | Composite snapshots | Strategy, Saturation, Outline |
| `FluidRadialImpulse` | Add impulses to fluid | Impulse Radius, Impulse Strength |
| `Text` | Render text | Position, FontSize, Colour |
| `Introspector` | Debug visualization | PointSize, Colours |

#### Layer Mods
| Type | Purpose | Key Parameters |
|------|---------|----------------|
| `Fade` | Fade layer toward background | Alpha (0.001-0.1 typical) |

### 2.4 Connection Syntax

Connections define data flow between mods:
```json
"connections": [
  "SourceMod.OutputName -> SinkMod.InputName"
]
```

Common connection patterns:
```json
// Audio to visual
"Audio.PitchRmsPoint -> Circles.Points"

// Camera to visual
"Camera.PointVelocity -> Particles.Points"

// Clustering
"Audio.PolarSpectral2dPoint -> Clusters.Vec2"
"Clusters.ClusterCentreVec2 -> Divisions.MajorAnchor"

// Palette learning
"Audio.Spectral3dPoint -> Palette.Vec3"
"Palette.Random -> ParticleField.PointColour"

// Fluid field driving particles
"Fluid.VelocitiesTexture -> ParticleField.Field1Fbo"
```

### 2.5 Fade Rate Guidelines

The `Alpha` parameter in FadeMod controls persistence:

| Alpha Value | Persistence | Use Case |
|-------------|-------------|----------|
| 0.001-0.002 | Very long (minutes) | Final marks, maximum accumulation |
| 0.003-0.005 | Long (30-60 sec) | Slow fade sections |
| 0.008-0.01 | Medium (15-30 sec) | Standard accumulation |
| 0.015-0.02 | Short (5-15 sec) | Active sections with turnover |
| 0.05-0.1 | Very short (1-5 sec) | Transient highlights |

### 2.6 Configuration Naming Convention

Use this naming pattern:
```
movement[N]-[movement-name]-[section-letter or variant].json
```

Examples:
- `movement1-source-a.json` - Movement 1, Section A
- `movement3-cataract-a-intensified.json` - Variant of Section A
- `movement4-meander-base.json` - Base configuration for variations
- `movement5-delta-coda.json` - Final section

---

## Part 3: Creating the Cue Sheet

### 3.1 Cue Sheet Header

```markdown
# PERFORMANCE CUE SHEET
## [Performance Title]

**Total Duration**: ~[X] minutes
**Performers**: [Number and type]
**Technical Setup**: [Brief description]

---
```

### 3.2 Movement Sections

For each movement:
```markdown
## MOVEMENT [N]: "[TITLE]" — [duration]

### Section [Letter] ([start time] - [end time])
- **Config**: `[filename].json`
- **Performers**: [Who and how many]
- **Visual**: [Key visual elements, brief]
- **Action**: [What performers do]
- **Fade**: [Rate if notable]

[If transition]:
**→ [TRANSITION TYPE] ([duration])**

### [Next Section]
...

**→ CAPTURE at [timestamp]** — "[Capture Name]"
```

### 3.3 Quick Reference Table

```markdown
## QUICK REFERENCE

| Movement | Duration | Performers | Energy | Config Count |
|----------|----------|------------|--------|--------------|
| 1: [Name] | X min | N | [Level] | N configs |
| 2: [Name] | X min | N | [Level] | N configs |
...
```

### 3.4 Capture Checklist

```markdown
## CAPTURE CHECKLIST

- [ ] Movement 1 at [time] — [Description]
- [ ] Movement 2 at [time] — [Description]
...
```

### 3.5 Technical Notes

```markdown
## TECHNICAL NOTES

### Pre-Performance
- Load config sequence
- Test camera/mic positioning
- Set display resolution
- Prepare capture storage

### During Performance
- Monitor layer states
- Watch for config transitions
- Prepare captures at timestamps

### Post-Performance
- Export captured images
- Save raw buffers
- Document any changes
- Archive performance data
```

---

## Part 4: Design Patterns

### 4.1 Progressive Complexity

Build complexity gradually:
1. **Start simple**: Single mod type, minimal layers
2. **Add elements**: Introduce new mods one at a time
3. **Peak**: Maximum layer count, all systems active
4. **Resolve**: Remove elements, return to simplicity

### 4.2 Layer Independence

Use separate layers for:
- Elements with different fade rates
- Different blend modes (alpha vs. additive)
- Data layers (velocities, etc.) vs. visible layers

### 4.3 Transitions

Types of transitions between sections:
- **Fade to Black**: Clean break, reset visual state
- **Crossfade**: Smooth blend between configurations
- **Hard Cut**: Immediate change (use for dramatic effect)
- **Persist Through**: Keep some layers while changing others

### 4.4 Energy Management

Match visual energy to musical energy:
- **Low energy**: Slow fades, sparse marks, minimal response
- **Building**: Increase density, add layers, faster response
- **Climax**: Maximum layers, high vorticity, full palette
- **Release**: Remove layers, increase fade rates, reduce density

### 4.5 Color Strategies

| Strategy | Implementation |
|----------|----------------|
| Monochromatic | Single hue with saturation/lightness variation |
| Complementary | Two opposing hues |
| Learned | SomPalette from audio spectral features |
| Temperature | Warm (low pitch) to cool (high pitch) mapping |
| White to color | Start white/pale, introduce color at climax |

---

## Part 5: Checklist for New Performances

### Conceptual Planning
- [ ] Central metaphor chosen
- [ ] Duration planned
- [ ] Movement count decided
- [ ] Energy arc mapped
- [ ] Capture points identified

### Technical Planning
- [ ] Input sources defined (mic, camera)
- [ ] Layer strategy for each movement
- [ ] Mod selection for each section
- [ ] Connection patterns documented
- [ ] Fade rates calibrated

### Documentation
- [ ] Performance plan complete
- [ ] All configuration files created
- [ ] Cue sheet prepared
- [ ] Technical checklist ready

### Testing
- [ ] Configurations load without error
- [ ] Transitions tested
- [ ] Capture workflow verified
- [ ] Full run-through completed

---

## Appendix: File Organization

```
docs/performance-[name]/
├── performance-[name].md           # Main performance plan
├── PERFORMANCE-CUE-SHEET.md       # Live cue sheet
└── performance-configs/
    ├── movement1-[name]-a.json
    ├── movement1-[name]-b.json
    ├── movement2-[name]-a.json
    └── ...
```

---

**Document Version**: 1.0  
**Created**: November 2025  
**For**: ofxMarkSynth Performance Planning
