# ofxMarkSynth Mods Reference

## Overview

ofxMarkSynth is a modular visual synthesizer that creates drawings in response to audio and video input. It uses a node-based architecture where **Mods** are connected together to form generative visual pipelines.

### Mod Architecture

Mods communicate through:
- **Sources**: Output data (points, scalars, textures, text, etc.)
- **Sinks**: Receive input data
- **Drawing Layers**: Shared FBO surfaces where mark-making happens

Data flows from **Source Mods** → **Process Mods** → **Sink Mods**, with **Layer Mods** applying effects to entire drawing surfaces.

---

## Source Mods

Source Mods generate data from various inputs (audio, video, random generation, text, timers).

### AudioDataSourceMod

Extracts musical features from audio (file or microphone) and emits them as points or scalars.

**Purpose**: Convert audio signals into visual data streams for drawing.

**Sources**:
- `PitchRmsPoint` (vec2): Pitch on X, loudness (RMS) on Y
- `PolarPitchRmsPoint` (vec2): Same data mapped to polar coordinates
- `Spectral2dPoint` (vec2): Spectral features as 2D points
- `PolarSpectral2dPoint` (vec2): Spectral features in polar coordinates
- `Spectral3dPoint` (vec3): 3D spectral representation
- `PitchScalar` (float): Normalized pitch value
- `RmsScalar` (float): Normalized loudness
- `ComplexSpectralDifferenceScalar` (float): Measure of spectral change
- `SpectralCrestScalar` (float): Spectral peak-to-average ratio
- `ZeroCrossingRateScalar` (float): Zero-crossing rate (relates to noisiness)
- `Onset1` (event): Audio onset detection
- `TimbreChange` (event): Timbre change detection
- `PitchChange` (event): Pitch change detection

**Key Parameters**:
- `MinPitch`, `MaxPitch`: Pitch range mapping (Hz)
- `MinRms`, `MaxRms`: Loudness range
- Audio analysis parameters for spectral features

**Use Cases**:
- Map pitch to Y position and loudness to size
- Trigger particle bursts on onsets
- Drive color palettes from spectral data

---

### VideoFlowSourceMod

Extracts motion flow data from video (file or camera).

**Purpose**: Convert video motion into drawing data.

**Sources**:
- `FlowFbo` (texture): Optical flow field as texture
- `PointVelocity` (vec2): Motion vectors at sampled points
- `Point` (vec2): Sampled motion point positions

**Key Parameters**:
- Motion detection sensitivity
- Flow field resolution

**Use Cases**:
- Drive particles with video motion
- Create fluid simulations from camera movement
- Extract velocity fields for smearing effects

---

### RandomFloatSourceMod

Generates random float values within a specified range.

**Purpose**: Add controlled randomness to any float parameter.

**Sources**:
- `Float` (float): Random float value

**Key Parameters**:
- `CreatedPerUpdate`: Number of floats generated per frame
- `Min`, `Max`: Value range

**Intent Integration**: Responds to Chaos dimension for generative variation.

**Use Cases**:
- Randomize size, opacity, or other scalar parameters
- Create stochastic variations in timing
- Drive probabilistic behaviors

---

### RandomVecSourceMod

Generates random vector positions (2D, 3D, or 4D).

**Purpose**: Create random spatial points for mark-making.

**Sources**:
- `Vec2` (vec2): 2D random positions (0-1 normalized)
- `Vec3` (vec3): 3D random positions
- `Vec4` (vec4): 4D random positions

**Key Parameters**:
- `CreatedPerUpdate`: Number of vectors per frame
- Vector dimensions (set at creation)

**Intent Integration**: Responds to Chaos dimension.

**Use Cases**:
- Generate random particle spawn points
- Create scattered mark positions
- Drive spatial variation

---

### RandomHslColorMod

Generates random colors using HSB (Hue, Saturation, Brightness) color space with controllable ranges.

**Purpose**: Create color variations within artistic constraints.

**Sources**:
- `Vec4` (vec4): RGBA color as vec4

**Sinks**:
- `ColorsPerUpdate` (float): Number of colors generated per frame
- `HueCenter` (float): Center of hue range (0-1)
- `HueWidth` (float): Width of hue range (0-1)
- `MinSaturation` (float): Minimum saturation
- `MaxSaturation` (float): Maximum saturation
- `MinBrightness` (float): Minimum brightness
- `MaxBrightness` (float): Maximum brightness
- `MinAlpha` (float): Minimum alpha
- `MaxAlpha` (float): Maximum alpha

**Key Parameters**:
- `CreatedPerUpdate`: Colors generated per frame
- `HueCenter`, `HueWidth`: Hue range (0-1, HueCenter uses angular interpolation)
- `MinSaturation`, `MaxSaturation`: Saturation bounds
- `MinBrightness`, `MaxBrightness`: Brightness bounds (HSB model)
- `MinAlpha`, `MaxAlpha`: Opacity bounds

**Intent Integration**: Responds to Chaos dimension.

**Use Cases**:
- Generate complementary color schemes
- Create monochromatic variations
- Add color diversity to particle systems
- Connect to RandomFloatSourceMod for agency-controlled parameter drift

---

### TextSourceMod

Reads text from files and emits words or lines sequentially or randomly.

**Purpose**: Generate text content for visual display.

**Sources**:
- `Text` (string): Text content

**Sinks**:
- `Next` (float): Trigger to emit next text item

**Key Parameters**:
- `TextFilename`: Source text file name
- `ParseWords`: Split into words (true) or keep lines (false)
- `Randomness`: 0.0 = sequential, 1.0 = random selection
- `Loop`: Whether to loop through content

**Intent Integration**: Chaos dimension controls randomness.

**Use Cases**:
- Display song lyrics synchronized to audio
- Create poetic text compositions
- Generate random word arrangements

---

### StaticTextSourceMod

Emits a fixed text string once or repeatedly.

**Purpose**: Display static text labels or messages.

**Sources**:
- `Text` (string): Static text content

**Key Parameters**:
- `Text`: The text string to emit
- `EmitOnce`: Emit only once vs. continuously
- `Delay`: Delay before emission (seconds)

**Use Cases**:
- Add titles or captions
- Display static labels
- Trigger text at specific times

---

### TimerSourceMod

Generates periodic tick events at regular intervals.

**Purpose**: Create rhythmic triggers for synchronization.

**Sources**:
- `Tick` (float): Emits value on each interval

**Sinks**:
- `Interval` (float): Set time between ticks
- `Enabled` (float): Enable/disable timer
- `OneShot` (float): Fire once then stop
- `TimeToNext` (float): Time until next tick
- `Start`, `Stop`, `Reset`, `TriggerNow`: Control commands

**Key Parameters**:
- `Interval`: Seconds between ticks (0.01-10.0)
- `Enabled`: Active/inactive state
- `OneShot`: Single trigger mode

**Intent Integration**: Responds to Tempo dimension.

**Use Cases**:
- Synchronize particle bursts to rhythm
- Trigger layer changes periodically
- Create metronome-like patterns

---

## Process Mods

Process Mods transform and manipulate data flowing through the synthesis pipeline.

### MultiplyAddMod

Applies linear transformation (multiply + add) to float values.

**Purpose**: Scale and offset numeric values.

**Sinks**:
- `Multiplier` (float): Multiplication factor
- `Adder` (float): Addition offset
- `Float` (float): Value to transform

**Sources**:
- `Float` (float): Transformed value

**Key Parameters**:
- `Multiplier`: Scale factor (-2.0 to 2.0)
- `Adder`: Offset value (-1.0 to 1.0)
- `Agency Factor`: Intent responsiveness (0.0-1.0)

**Intent Integration**: Full Intent system support with configurable agency.

**Use Cases**:
- Remap scalar ranges
- Invert values (negative multiplier)
- Add offsets to center or shift data

---

### ClusterMod

Groups 2D points into clusters using k-means algorithm and outputs cluster centers.

**Purpose**: Find spatial patterns and reduce point density.

**Sinks**:
- `Vec2` (vec2): Input points to cluster
- `ChangeClusterNum` (float): Adjust number of clusters

**Sources**:
- `ClusterCentreVec2` (vec2): Cluster center positions

**Key Parameters**:
- Cluster count (adjustable via sink)
- `Agency Factor`: Intent responsiveness

**Intent Integration**: Structure dimension affects clustering behavior.

**Use Cases**:
- Reduce audio point density to cleaner positions
- Find average positions for mark placement
- Create hierarchical spatial organization

---

### PathMod

Converts a stream of points into geometric paths using various strategies.

**Purpose**: Create shapes from point sequences.

**Sinks**:
- `Vec2` (vec2): Points to add to path

**Sources**:
- `Path` (ofPath): Generated path object

**Key Parameters**:
- `Strategy`: 0=polypath, 1=bounds, 2=horizontals, 3=convex hull
- `MaxVertices`: Maximum points in path (0-20)
- `MaxVertexProximity`: Maximum distance between adjacent vertices
- `MinVertexProximity`: Minimum distance between vertices

**Intent Integration**: Responds to Structure dimension.

**Use Cases**:
- Create polygonal shapes from audio points
- Generate convex hulls around particle clusters
- Build bounding boxes for collage sources

---

### PixelSnapshotMod

Captures rectangular regions from the drawing surface.

**Purpose**: Sample visual content for reuse or collaging.

**Sources**:
- `SnapshotTexture` (texture): Captured image region

**Key Parameters**:
- `SnapshotsPerUpdate`: Capture frequency (0.0-1.0)
- `Size`: Snapshot dimensions in pixels (128-8096)

**Intent Integration**: Responds to Chaos dimension for snapshot timing.

**Use Cases**:
- Extract regions for collage composition
- Create feedback loops
- Sample interesting visual moments

---

### SomPaletteMod

Self-organizing map that learns color palettes from 3D color input.

**Purpose**: Generate coherent color schemes from audio or other data.

**Sinks**:
- `Sample` (vec3): RGB color input for training
- `SwitchPalette` (float): Change active palette

**Sources**:
- `Random` (vec4): Random color from palette
- `RandomDark` (vec4): Random dark color
- `RandomLight` (vec4): Random light color
- `Darkest` (vec4): Darkest palette color
- `Field` (texture): 2D palette texture (16×16)

**Key Parameters**:
- `Iterations`: Training iterations (100-10000)

**Intent Integration**: Responds to Chaos/Structure for palette evolution.

**Use Cases**:
- Learn color palettes from album art
- Generate audio-reactive color schemes
- Create harmonious color variations

---

## Sink Mods (Mark-Making)

Sink Mods create visual marks on drawing layers. They can be categorized by their visual characteristics.

### Soft/Organic Marks

#### SoftCircleMod

Draws soft, alpha-blended circles with Gaussian falloff.

**Visual Characteristics**: Soft, glowing, ethereal • Organic edges • Overlapping creates luminosity

**Sinks**:
- `Points` (vec2): Circle center positions
- `Radius` (float): Circle size
- `Colour` (vec4): Circle color
- `ColourMultiplier` (float): Color intensity multiplier
- `AlphaMultiplier` (float): Opacity multiplier
- `Softness` (float): Edge falloff amount

**Key Parameters**:
- `Radius`: Circle size (0.0-0.25 normalized)
- `Colour`: Base color (RGBA)
- `ColourMultiplier`: RGB intensity (0.0-1.0)
- `AlphaMultiplier`: Transparency (0.0-1.0)
- `Softness`: Edge blur (0.0-1.0)
- `Falloff`: Falloff mode (0=Glow, 1=Dab)
- `AgencyFactor`: Intent responsiveness

**Falloff Modes**:
- `0` (Glow): Exponential falloff creating sharp-centered, glowing marks. Good for dark backgrounds where marks add light.
- `1` (Dab): Quadratic falloff for broader, softer marks with premultiplied alpha compositing. Marks accumulate naturally without halo artifacts when overlapping. Works well on any background color.

**Intent Integration**: Full Intent support with agency control.

**Use Cases**:
- Create soft atmospheric effects
- Draw glowing audio-reactive particles
- Build up luminous compositions through layering
- Use Falloff=1 (Dab) for subtle marks on white backgrounds

---

#### ParticleSetMod

Physics-based particle system with attraction, connections, and trails.

**Visual Characteristics**: Organic motion • Dynamic • Connective lines • Physics-driven behavior

**Sinks**:
- `Point` (vec2): Particle spawn position
- `PointVelocity` (vec2): Particle with initial velocity (vec4: x,y,dx,dy)
- `Spin` (float): Rotation speed
- `Colour` (vec4): Particle color

**Key Parameters**:
- `Spin`: Particle rotation (-0.05 to 0.05)
- `Colour`: Particle color
- `timeStep`: Physics simulation speed
- `velocityDamping`: Movement friction
- `attractionStrength`: Inter-particle attraction
- `attractionRadius`: Range of attraction
- `connectionRadius`: Distance for drawing connections
- `maxSpeed`: Velocity cap
- `AgencyFactor`: Intent responsiveness

**Intent Integration**: Full Intent with agency control.

**Use Cases**:
- Create flowing, organic particle clouds
- Visualize audio motion with physics
- Build interconnected network visualizations

---

#### ParticleFieldMod

Particle system driven by two texture-based vector fields (like fluid velocities).

**Visual Characteristics**: Field-guided motion • Smooth, flowing trajectories • Complex patterns

**Sinks**:
- `Field1Fbo` (texture): Primary vector field (RG channels)
- `Field2Fbo` (texture): Secondary vector field
- `PointColor` (vec4): Update particle colors
- `MinWeight` (float): Field 1 influence minimum
- `MaxWeight` (float): Field 1 influence maximum

**Key Parameters**:
- Field weights control blend between two fields
- Inherits particle rendering parameters

**Intent Integration**: Responds to Intent system.

**Use Cases**:
- Create fluid-driven particle motion
- Combine video flow with fluid simulation
- Generate complex field-guided patterns

---

#### SandLineMod

Draws lines as distributed point grains with Gaussian distribution.

**Visual Characteristics**: Textural • Grainy • Soft edges • Sand-like quality • Hand-drawn feel

**Sinks**:
- `Point` (vec2): Line endpoints (pairs of points)
- `PointRadius` (float): Grain size
- `Colour` (vec4): Grain color

**Key Parameters**:
- `Density`: Grains per unit length (0.0-0.5)
- `PointRadius`: Individual grain size (0.0-32.0 pixels)
- `Colour`: Base color
- `AlphaMultiplier`: Transparency (0.0-1.0)
- `StdDevAlong`: Distribution along line (0.0-1.0)
- `StdDevPerpendicular`: Distribution perpendicular to line (0.0-0.1)

**Intent Integration**: Responds to Intent dimensions.

**Use Cases**:
- Create textural, organic line work
- Draw audio-reactive sand patterns
- Build up grainy, atmospheric compositions

---

### Hard/Geometric Marks

#### DividedAreaMod

Divides the drawing space with lines using various geometric strategies.

**Visual Characteristics**: Geometric • Sharp edges • Architectural • Grid-like divisions • Refraction effects

**Sinks**:
- `MajorAnchor` (vec2): Major division line anchor points
- `MinorAnchor` (vec2): Minor division line anchors
- `MinorPath` (ofPath): Path for line generation
- `MinorLineColour` (vec4): Minor line color
- `MajorLineColour` (vec4): Major line color
- `BackgroundFbo` (texture): Source for refraction on major lines
- `ChangeAngle`, `ChangeStrategy`, `ChangeLayer`: Control commands

**Key Parameters**:
- `Strategy`: 0=point pairs, 1=point angles, 2=radiating
- `Angle`: Line angle for strategy 1 (0.0-0.5, in π radians)
- `PathWidth`: Minor line width
- `MajorLineWidth`: Major line width (pixels)
- `MinorLineColour`, `MajorLineColour`: Line colors
- `MaxUnconstrainedLines`: Maximum divisions

**Intent Integration**: Responds to Structure dimension.

**Use Cases**:
- Create geometric compositions responding to audio
- Build architectural space divisions
- Generate grid-based visualizations

---

#### CollageMod

Composites texture snapshots within path boundaries.

**Visual Characteristics**: Hard-edged shapes • Collage aesthetic • Textural fills • Outlined forms

**Sinks**:
- `Path` (ofPath): Shape boundary
- `SnapshotTexture` (texture): Image to collage
- `Colour` (vec4): Tint color

**Key Parameters**:
- `Colour`: Tint color for textures
- `Saturation`: Color saturation boost (0.0-4.0)
- `Outline`: Outline vs fill blend (0.0-1.0)
- `Strategy`: 0=tint, 1=add tinted pixels, 2=add pixels

**Layers**:
- Default layer: Filled shapes
- `outlines` layer: Shape outlines

**Intent Integration**: Responds to Intent.

**Use Cases**:
- Create cut-paper collage effects
- Composite video snapshots into shapes
- Build layered textural compositions

---

### Text Marks

#### TextMod

Renders text strings using TrueType fonts.

**Visual Characteristics**: Typographic • Clear letterforms • Positioned text

**Sinks**:
- `Text` (string): Text content to display
- `Position` (vec2): Text position (normalized 0-1)
- `FontSize` (float): Size multiplier
- `Colour` (vec4): Text color
- `Alpha` (float): Opacity

**Key Parameters**:
- `Position`: Text location (0-1 normalized coordinates)
- `FontSize`: Size as proportion of canvas (0.01-0.5)
- `Colour`: RGBA color
- `Alpha`: Transparency (0.0-1.0)
- `AgencyFactor`: Intent responsiveness

**Intent Integration**: Full Intent support with agency.

**Use Cases**:
- Display lyrics synchronized to music
- Create typographic compositions
- Add textual annotations to visuals

---

### Fluid/Dynamic Marks

#### FluidRadialImpulseMod

Adds radial velocity impulses to a fluid simulation layer.

**Visual Characteristics**: Fluid distortion • Ripple effects • Dynamic flow patterns

**Sinks**:
- `Points` (vec2): Impulse centers
- `ImpulseRadius` (float): Impulse size
- `ImpulseStrength` (float): Force magnitude

**Key Parameters**:
- `Impulse Radius`: Size of force field (0.0-0.3)
- `Impulse Strength`: Force magnitude (0.0-0.3)

**Intent Integration**: Responds to Intent.

**Use Cases**:
- Create fluid ripples from audio onsets
- Add energy to fluid simulations
- Generate dynamic flow disturbances

---

### Debug/Visualization

#### IntrospectorMod

Visualizes data points and horizontal lines for debugging.

**Visual Characteristics**: Debug overlay • Non-persistent (separate FBO) • Colored traces

**Sinks**:
- `Points` (vec2): Points to visualize
- `HorizontalLines1`, `HorizontalLines2`, `HorizontalLines3` (float): Y-positions for lines

**Key Parameters**:
- `PointSize`: Visualization point size (0.0-4.0)
- `PointFade`: Frames until point fades
- `Colour`: Point color
- `HorizontalLineFade`: Frames until line fades
- `HorizontalLine1Color`, etc.: Line colors

**Note**: Uses separate FBO, not visible in saved output.

**Use Cases**:
- Debug audio feature extraction
- Visualize data flow
- Monitor parameter ranges

---

## Layer Mods

Layer Mods apply effects to entire drawing surfaces rather than making individual marks.

### FluidMod

2D fluid simulation that creates flowing, organic effects on a layer.

**Visual Characteristics**: Flowing • Organic motion • Vortex formation • Smooth blending

**Sources**:
- `VelocitiesTexture` (texture): Velocity field texture

**Layers**:
- Default layer: Visual simulation output
- `velocities` layer: Velocity field (can drive ParticleFieldMod)

**Key Parameters**:
- `dt`: Time step (simulation speed)
- `vorticity`: Vortex formation strength
- `valueDissipation`: Color/value fade rate
- `velocityDissipation`: Motion decay rate
- `AgencyFactor`: Intent responsiveness

**Intent Integration**: Full Intent support with agency.

**Use Cases**:
- Create flowing organic backgrounds
- Add fluid motion to particle systems
- Generate evolving abstract patterns

---

### FadeMod

Gradually fades layer content over time using alpha blending.

**Visual Characteristics**: Gradual fade • Trails • Temporal blending

**Sinks**:
- `AlphaMultiplier` (float): Override fade rate

**Key Parameters**:
- `Alpha`: Per-frame alpha multiplier (0.0-0.1)

**Intent Integration**: Responds to Intent.

**Use Cases**:
- Create trailing effects
- Gradually clear the canvas
- Build temporal layers

---

### SmearMod

Applies displacement-based smearing using vector fields with various spatial strategies.

**Visual Characteristics**: Smeared/distorted • Echo effects • Displacement patterns • Glitchy artifacts (depending on strategy)

**Sinks**:
- `Vec2` (vec2): Direct displacement offset
- `Float` (float): Scalar multiplier for displacement
- `Field1Tex` (texture): Primary displacement field (RG channels)
- `Field2Tex` (texture): Secondary displacement field

**Key Parameters**:
- `MixNew`: Blend with previous frame (0.3-1.0)
- `AlphaMultiplier`: Fade rate (0.994-0.999)
- `Translation`: Direct translation offset
- `Field1Multiplier`, `Field1Bias`: Primary field control (0.0-0.05)
- `Field2Multiplier`, `Field2Bias`: Secondary field control
- `Strategy`: Spatial manipulation mode (0-9):
  - 0: Off
  - 1: Cell-quantized
  - 2: Per-cell random offset
  - 3: Boundary teleport
  - 4: Per-cell rotation/reflection
  - 5: Multi-res grid snap
  - 6: Voronoi partition teleport
  - 7: Border kill-band
  - 8: Dual-sample ghosting on border cross
  - 9: Piecewise mirroring/folding
- `GridSize`: Grid dimensions for strategies (2-128)
- Strategy-specific parameters (jump amount, border width, ghost blend, fold period)

**Intent Integration**: Responds to Intent.

**Use Cases**:
- Create fluid-like smearing from velocity fields
- Generate glitch effects with teleport/fold strategies
- Build feedback loops with spatial distortion
- Combine video flow for video-reactive smearing

---

## Conceptual Synth Configurations

Below are example configurations that combine Mods to create expressive visual performances.

### 1. "Audio Sandstorm"
**Aesthetic**: Textural, organic, grainy visualization of music

```
AudioDataSourceMod (audio file)
  └─ PitchRmsPoint → SandLineMod
  └─ RmsScalar → SandLineMod.PointRadius

RandomHslColorMod
  └─ Vec4 → SandLineMod.Colour

FadeMod (slow fade for accumulation)
```

**Performance Idea**: Dense, textural mark-making builds up like sand dunes responding to musical dynamics.

---

### 2. "Flowing Particles"
**Aesthetic**: Organic, physics-based motion with fluid dynamics

```
AudioDataSourceMod (microphone)
  └─ Onset1 → RandomVecSourceMod.trigger

RandomVecSourceMod
  └─ Vec2 → ParticleSetMod.Point

FluidMod (generates velocity field)
  └─ VelocitiesTexture → ParticleFieldMod.Field1Fbo

ParticleFieldMod (particles follow fluid)

SomPaletteMod (learns colors from audio)
  └─ Random → ParticleFieldMod.PointColor
```

**Performance Idea**: Particles spawn on audio onsets and flow through fluid simulations, with colors learned from the music's spectral content.

---

### 3. "Geometric Sound Architecture"
**Aesthetic**: Clean, architectural, grid-based

```
AudioDataSourceMod
  └─ PolarPitchRmsPoint → ClusterMod.Vec2

ClusterMod (reduces to 5-8 clusters)
  └─ ClusterCentreVec2 → DividedAreaMod.MajorAnchor

DividedAreaMod (Strategy: point angles)
  └─ Major lines divide space geometrically

TimerSourceMod (triggers every 2 seconds)
  └─ Tick → DividedAreaMod.ChangeAngle

FadeMod (very slow fade)
```

**Performance Idea**: Audio features create geometric space divisions that evolve over time, building architectural compositions.

---

### 4. "Text Collage Dreams"
**Aesthetic**: Collage of text and captured visual moments

```
TextSourceMod (poetry file, randomness=0.5)
  └─ Text → TextMod.Text

TimerSourceMod (interval=3.0s)
  └─ Tick → TextSourceMod.Next
  └─ Tick → PixelSnapshotMod.trigger

RandomVecSourceMod (slow creation rate)
  └─ Vec2 → PathMod.Vec2

PathMod (Strategy: convex hull)
  └─ Path → CollageMod.Path

PixelSnapshotMod
  └─ SnapshotTexture → CollageMod.SnapshotTexture

RandomHslColorMod (narrow hue range)
  └─ Vec4 → CollageMod.Colour

SmearMod (Strategy: boundary teleport)
```

**Performance Idea**: Text appears, visual moments are captured and collaged into geometric shapes, with spatial distortion creating dreamlike effects.

---

### 5. "Glowing Audio Cloud"
**Aesthetic**: Soft, atmospheric, luminous

```
AudioDataSourceMod
  └─ PitchRmsPoint → SoftCircleMod.Points
  └─ ComplexSpectralDifferenceScalar → MultiplyAddMod.Float

MultiplyAddMod (scale spectral change to radius)
  └─ Float → SoftCircleMod.Radius

SomPaletteMod
  └─ RandomLight → SoftCircleMod.Colour

FadeMod (medium fade rate)
```

**Performance Idea**: Soft glowing marks accumulate in response to audio, with size driven by spectral change and colors from a learned palette.

---

### 6. "Video Motion Trails"
**Aesthetic**: Camera-driven, organic motion, smeared trails

```
VideoFlowSourceMod (camera input)
  └─ PointVelocity → ParticleSetMod.PointVelocity
  └─ FlowFbo → SmearMod.Field1Tex

ParticleSetMod (particles follow camera motion)

SmearMod (Strategy: off, just field-based smearing)

RandomHslColorMod
  └─ Vec4 → ParticleSetMod.Colour

FadeMod (subtle fade)
```

**Performance Idea**: Camera movement generates particles and smears the canvas, creating organic motion trails that respond to physical movement.

---

### 7. "Rhythmic Impulse Fluid"
**Aesthetic**: Pulsing fluid dynamics synchronized to rhythm

```
AudioDataSourceMod
  └─ Onset1 → RandomVecSourceMod.trigger

RandomVecSourceMod
  └─ Vec2 → FluidRadialImpulseMod.Points

FluidRadialImpulseMod (adds energy to fluid)

FluidMod (creates flowing patterns)
  └─ VelocitiesTexture → SmearMod.Field1Tex

SoftCircleMod (draws on fluid layer)

SmearMod (smears based on fluid motion)

TimerSourceMod (4-beat pattern)
  └─ Tick → FluidMod.Layer.Clear
```

**Performance Idea**: Audio onsets create fluid impulses that flow and swirl, with periodic clearing creating rhythmic visual phrases.

---

### 8. "Clustered Division"
**Aesthetic**: Audio-reactive spatial partitioning

```
AudioDataSourceMod
  └─ Spectral2dPoint → ClusterMod.Vec2

ClusterMod (8 clusters)
  └─ ClusterCentreVec2 → DividedAreaMod.MinorAnchor

AudioDataSourceMod
  └─ Onset1 → ClusterMod.ChangeClusterNum

DividedAreaMod (Strategy: radiating)

PixelSnapshotMod
  └─ SnapshotTexture → DividedAreaMod.BackgroundFbo (refraction)

SomPaletteMod
  └─ RandomDark → DividedAreaMod.MinorLinesColour
```

**Performance Idea**: Audio clusters create radiating line divisions, with changing cluster counts on beats and color schemes learned from the music.

---

## Tips for Expressive Performance

1. **Layering**: Use multiple drawing layers with different blend modes to create depth
2. **Timing**: Combine `TimerSourceMod` with layer changes for rhythmic visual phrases
3. **Color Coherence**: Use `SomPaletteMod` to maintain color harmony while adding variation
4. **Spatial Hierarchy**: Chain `ClusterMod` → geometric mods to organize chaos
5. **Intent System**: Map controllers to Intent dimensions for expressive real-time control
6. **Feedback Loops**: Connect `PixelSnapshotMod` → processing → drawing for self-referential systems
7. **Dynamic Parameters**: Use `RandomFloatSourceMod` or audio scalars to modulate any parameter
8. **Clearing Strategies**: Vary `FadeMod` rates or use timed clears for visual phrasing

---

## Parameter Modulation via Intent System

Many Mods support the **Intent System** for expressive real-time control. The Intent system maps external controllers to abstract dimensions:

- **Chaos**: Randomness, entropy, disorder
- **Structure**: Order, organization, geometry
- **Tempo**: Speed, rhythm, pace
- **Energy**: Intensity, force, magnitude

Mods with `AgencyFactor` parameters can control their responsiveness to global Intent changes. Controllers can be mapped to Mod parameters through the `ParamController` system, enabling smooth interpolation and complex mappings.

---

**Document Version**: 1.2
**Last Updated**: November 30, 2025
