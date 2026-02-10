# ofxMarkSynth Mods Reference

## Overview

ofxMarkSynth is a modular visual synthesizer that creates drawings in response to audio and video input. It uses a node-based architecture where **Mods** are connected together to form generative visual pipelines.

### Mod Architecture

Mods communicate through:
- **Sources**: Output data (points, scalars, textures, text, etc.)
- **Sinks**: Receive input data
- **Drawing Layers**: Shared FBO surfaces where mark-making happens

Data flows from **Source Mods** → **Process Mods** → **Sink Mods**, with **Layer Mods** applying effects to entire drawing surfaces.

See also:
- `docs/performer-guide.md`: how to play MarkSynth live (MIDI-centric)
- `docs/recipes.md`: wiring patterns + tuning “cookbook”

---

## Source Mods

Source Mods generate data from various inputs (audio, video, random generation, text, timers).

### AudioDataSourceMod

Extracts musical features from the Synth-owned audio analysis client and emits them as points or scalars.

**Purpose**: Convert audio analysis features into visual data streams for drawing.

**Audio Ownership**:
- Audio input (file or microphone) is configured at Synth creation time via `ResourceManager` (see `docs/SYNTH-RESOURCES.md`).
- `AudioDataSourceMod` does not open/close the stream or manage segment recording; `Synth::toggleRecording()` controls audio segment recording.

**Sources**:
- `PitchRmsPoint` (vec2): Pitch on X, loudness (RMS) on Y
- `PolarPitchRmsPoint` (vec2): Same data mapped to polar coordinates
- `DriftPitchRmsPoint` (vec2): Pitch on X, RMS on Y with quiet-passages hold+drift + wrap
- `Spectral2dPoint` (vec2): Spectral features as 2D points
- `PolarSpectral2dPoint` (vec2): Spectral features in polar coordinates
- `DriftSpectral2dPoint` (vec2): Spectral centroid on X, crest on Y with quiet-passages hold+drift + wrap
- `Spectral3dPoint` (vec3): 3D spectral representation
- `PitchScalar` (float): Normalized pitch value
- `RmsScalar` (float): Normalized loudness
- `ComplexSpectralDifferenceScalar` (float): Measure of spectral change
- `SpectralCrestScalar` (float): Spectral peak-to-average ratio
- `ZeroCrossingRateScalar` (float): Zero-crossing rate (relates to noisiness)
- `Onset1` (event): Audio onset detection (sudden energy + spectral change)
- `TimbreChange` (event): Timbre change detection (spectral crest + zero-crossing rate change)
- `PitchChange` (event): Pitch change detection

**Key Parameters**:
- `MinPitch`, `MaxPitch`: Pitch range mapping (Hz)
- `MinRms`, `MaxRms`: Loudness range
- Audio analysis parameters for spectral features

**Event Detection Tuning** (runtime-adjustable for live performance):
- `OnsetThreshold` (0.5-5.0, default 2.0): Z-score threshold for onset detection. Lower = more sensitive (more events)
- `OnsetCooldownSecs` (0.0-30.0, default 2.0): Minimum seconds between onset events
- `TimbreThreshold` (0.5-5.0, default 2.0): Z-score threshold for timbre change detection
- `TimbreCooldownSecs` (0.0-30.0, default 2.0): Minimum seconds between timbre events
- `PitchThreshold` (0.5-5.0, default 2.0): Z-score threshold for pitch change detection
- `PitchCooldownSecs` (0.0-30.0, default 2.0): Minimum seconds between pitch events

**Use Cases**:
- Map pitch to Y position and loudness to size
- Trigger particle bursts on onsets
- Drive color palettes from spectral data

---

### VideoFlowSourceMod

Extracts motion flow data from video (file or camera).

**Purpose**: Convert video motion into drawing data.

**Sources**:
- `FlowField` (texture): Optical flow field as texture
- `PointVelocity` (vec2): Motion vectors at sampled points
- `Point` (vec2): Sampled motion point positions

**Key Parameters**:
- `PointSamplesPerUpdate`: Number of motion point samples emitted per frame (0-500)
- Motion detection sensitivity
- Flow field resolution
- `AgencyFactor`: Scales this Mod’s effective Synth Agency for auto-wired parameters (0.0–1.0). Does not scale Intent strength.

**Intent Integration**: Density (D) increases `PointSamplesPerUpdate` via exp(0.5).

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

**Hue Color Wheel Reference** (normalised 0-1):
| Value | Color   |
|-------|---------|
| 0.00  | Red     |
| 0.08  | Orange  |
| 0.17  | Yellow  |
| 0.33  | Green   |
| 0.50  | Cyan    |
| 0.58  | Azure   |
| 0.67  | Blue    |
| 0.75  | Violet  |
| 0.83  | Magenta |
| 0.92  | Rose    |
| 1.00  | Red (wraps) |

**Intent Integration**: Responds to Chaos dimension.

**Use Cases**:
- Generate complementary color schemes
- Create monochromatic variations
- Add color diversity to particle systems
- Connect to RandomFloatSourceMod for agency-controlled parameter drift

---

### TextSourceMod

Reads text from files and emits words or lines sequentially or randomly, with proper exhaustion handling.

**Purpose**: Generate text content for visual display.

**Sources**:
- `Text` (string): Text content

**Sinks**:
- `Next` (float): Trigger to emit next text item

**Key Parameters**:
- `TextFilename`: Source text file name
- `ParseWords`: Split into words (true) or keep lines (false)
- `Randomness`: 0.0 = sequential, 1.0 = random selection, 0.5 = 50% chance of each
- `Loop`: Controls behavior after all items emitted:
  - `true`: Resets and continues emitting indefinitely
  - `false`: Stops emitting after each item has been emitted exactly once

**Exhaustion Behavior**:
- Tracks which items have been emitted (works for both sequential and random modes)
- In mixed mode (`Randomness` between 0 and 1), both sequential and random picks draw from the same pool of unvisited items
- When `Loop` is toggled from `false` to `true`, emission state resets immediately

**Intent Integration**: Chaos dimension controls randomness.

**Use Cases**:
- Display song lyrics synchronized to audio (sequential, no loop)
- Create poetic text compositions
- Generate random word arrangements (random, with loop)
- Emit each line exactly once in random order (random, no loop)

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
- `float` (float): Value to transform

**Sources**:
- `float` (float): Transformed value

**Key Parameters**:
- `Multiplier`: Scale factor (-2.0 to 2.0)
- `Adder`: Offset value (-1.0 to 1.0)
- `AgencyFactor`: Auto takeover scaling (multiplies Synth Agency) (0.0-1.0)

**Intent Integration**: None currently (no `applyIntent()` mapping).

**Use Cases**:
- Remap scalar ranges
- Invert values (negative multiplier)
- Add offsets to center or shift data

---

### FadeAlphaMapMod

Maps an arbitrary float input to a time-based half-life in seconds.

This is a compatibility-friendly way to drive `FadeMod.HalfLifeSec` (or `SmearMod.HalfLifeSec`) from a scalar like `Audio.RmsScalar`.

**Sinks**:
- `Multiplier` (float): Linear mapping coefficient
- `Adder` (float): Linear mapping offset
- `float` (float): Input value to map

**Sources**:
- `float` (float): Output half-life in seconds

**Key Parameters**:
- `Multiplier`, `Adder`: First compute a legacy alpha-per-frame signal: `alphaPerFrame = Multiplier*float + Adder`
- `ReferenceFps`: FPS used to interpret `alphaPerFrame` (default 30)
- `MinHalfLifeSec`, `MaxHalfLifeSec`: Clamp output range
- `AgencyFactor`: Auto takeover scaling (multiplies Synth Agency)

**Intent Integration**: None currently (no `applyIntent()` mapping).

**Use Cases**:
- Audio dynamics → layer persistence:
  - `Audio.RmsScalar → FadeAlphaMap.float → FadeMod.HalfLifeSec`
- Same mapping for `SmearMod.HalfLifeSec`

---

### VectorMagnitudeMod

Computes a normalized scalar magnitude from incoming vectors, and exposes both a mean and a peak signal.

**Purpose**: Turn vector streams (e.g. video motion velocity, 3D tracking) into stable 0–1 scalar envelopes.

**Sinks**:
- `Vec2` (vec2): input vectors
- `Vec3` (vec3): input vectors
- `Vec4` (vec4): input vectors
- `PointVelocity` (vec4): alias for `Vec4` (useful with `VideoFlowSourceMod.PointVelocity`)

**Sources**:
- `MeanScalar` (float): mean magnitude per update (smoothed)
- `MaxScalar` (float): max magnitude per update (smoothed)

**Key Parameters**:
- `Components`: which components to measure
  - `"xy"`, `"zw"`, `"xyz"`, `"xyzw"`
- `Min`, `Max`: normalization range for the selected magnitude
- `MeanSmoothing`, `MaxSmoothing`: smoothing factors (0=immediate, 1=held)
- `DecayWhenNoInput`: decay toward 0 when no samples arrive

**Intent Integration**: None (currently).

**Use Cases**:
- Derive a "video RMS" from `VideoFlowSourceMod.PointVelocity`.
- Drive agency-aware sinks (fade rate, alpha multipliers, impulse strength) from motion intensity.
- Convert 3D tracking vectors into stable scalar control signals.

---

### ClusterMod

Groups 2D points into clusters using k-means algorithm and outputs cluster centers.

**Purpose**: Find spatial patterns and reduce point density.

**Sinks**:
- `Vec2` (vec2): Input points to cluster
- `ChangeClusterCount` (float): Adjust number of clusters

**Sources**:
- `ClusterCentre` (vec2): Cluster center positions

**Key Parameters**:
- Cluster count (adjustable via sink)
- `AgencyFactor`: Auto takeover scaling (multiplies Synth Agency)

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
- `Point` (vec2): Points to add to path
- `Trigger` (float): Trigger path emission (only available in trigger-based mode)

**Sources**:
- `Path` (ofPath): Generated path object

**Config Options** (set in JSON, not runtime parameters):
- `TriggerBased`: When `"true"`, PathMod only emits paths when triggered via the `Trigger` sink. Points accumulate continuously and are clustered on trigger. Points outside the cluster are retained for subsequent triggers. When `"false"` (default), paths are emitted automatically when enough clustered points accumulate.

**Key Parameters**:
- `Strategy`: 0=polypath, 1=bounds, 2=horizontals, 3=convex hull
- `MaxVertices`: Maximum points in path (0-20). In trigger-based mode, up to 7× this value are retained.
- `ClusterRadius`: Maximum distance from the newest point for inclusion in the cluster (0.01-0.5 normalized). Points within this radius form the cluster that gets shaped by the Strategy. (Intent mapping additionally caps this to ~0.4 to prevent screen-sized paths.)
- `MinClusterPoints`: Minimum clustered points required before emitting a path (default 4). Useful to prevent tiny/degenerate paths when triggers are frequent or point sources are stable.
- `MinBoundsSize`: Minimum bounding-box extent (max of width/height) required before emitting a path (default 0.02). Useful when you want to avoid tiny paths that disappear into textured layers like Smear.

**Intent Integration**: Responds to Granularity (cluster radius) and Density (max vertices) dimensions.

**Modes**:
- **Continuous mode** (default): Automatically emits a path when `MinClusterPoints` points cluster within `ClusterRadius` (defaults to 4). All points are cleared after emission.
- **Trigger-based mode**: Points accumulate continuously. On receiving a trigger, emits a path from clustered points and retains non-clustered points for the next trigger. This mode is useful for synchronizing path emission with other events (e.g., audio onsets, memory bank emissions).

**Use Cases**:
- Create polygonal shapes from audio points
- Generate convex hulls around particle clusters
- Build bounding boxes for collage sources
- Synchronize path generation with audio events (trigger-based mode)

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

Self-organizing map that learns palettes from incoming `Sample` colors and emits multiple palette-derived outputs.

**Purpose**: Generate coherent (but evolving) color schemes from audio/video features or other signals.

**Sinks**:
- `Sample` (vec3): RGB training sample (0–1). Typical sources: `Audio.Spectral3dPoint`, `Camera/Video` color samples, or any vec3 stream.
- `SwitchPalette` (float): When > 0.5 and a “next” palette is ready, switch to it (edge-trigger style control).

**Sources**:
- `Random` (vec4): Random RGBA from the learned palette.
- `RandomNovelty` (vec4): Random RGBA that prefers a short-lived “novelty cache” when available (falls back to `Random`).
- `RandomDark` (vec4): Random RGBA biased toward darker chips.
- `RandomLight` (vec4): Random RGBA biased toward lighter chips.
- `Darkest` (vec4): The darkest chip.
- `Lightest` (vec4): The lightest chip.
- `FieldTexture` (texture): Small RG texture derived from the palette (currently `GL_RG16F`, 8×8 or 16×16 depending on implementation). Often useful as a weak vector field for `Smear`/`Fluid`/`ParticleField`.

**Key Parameters**:
- Convergence + responsiveness:
  - `Iterations`: Training iterations per palette (higher = more stable, slower to adapt).
  - `TrainingStepsPerFrame`: How hard we train each frame (higher = faster adaptation, more “nervous”).
- Window + memory:
  - `WindowSecs`: Sliding feature window length in seconds (higher = smoother, lower = twitchier).
  - `ChipMemoryMultiplier`: Persistence multiplier on top of `WindowSecs` (higher = longer-lived palette identity).
- Startup behavior:
  - `StartupFadeSecs`: Fade-in from first received sample (avoids harsh flashes on load).
- Novelty:
  - `RandomNovelty` prefers a short-lived cache of “outlier” colors sampled from the full SOM field (audio-derived), falling back to `Random` when the cache is empty.
  - The cache is designed to stay meaningfully different from the main (persistent) palette: cached entries are only refreshed when they reappear as genuine outliers, so they don’t slowly drift back toward baseline colors.
  - Very neutral / near-gray candidates are filtered out; dark-but-saturated colors are still eligible.
  - `NoveltyEmitChance`: Chance that `Random`, `RandomDark`, and `RandomLight` occasionally emit a cached novelty color (when available).
- Anti-collapse (for drones / low-variance inputs):
  - `AntiCollapseJitter`: Amount of injected variation when the input feature history has very low variance.
  - `AntiCollapseVarianceSecs`: Lookback window for variance measurement.
  - `AntiCollapseVarianceThreshold`: Variance threshold below which anti-collapse activates.
  - `AntiCollapseDriftSpeed`: Speed of the injected drift.
- Colorizer:
  - `ColorizerGrayGain`, `ColorizerChromaGain`: Balance gray/chroma emphasis in palette generation.
- `AgencyFactor`: Currently unused (reserved for future intent/controller blending).

**Coordination rules**:
- If the palette changes too fast: increase `WindowSecs`, decrease `TrainingStepsPerFrame`, and/or increase `ChipMemoryMultiplier`.
- If the palette feels “stuck”: decrease `WindowSecs`, increase `TrainingStepsPerFrame`, and consider increasing `NoveltyEmitChance`.
- If sustained tones collapse the palette: increase `AntiCollapseJitter` slightly and ensure `AntiCollapseVarianceSecs` is not too short.

**Intent Integration**: None currently (`SomPaletteMod::applyIntent()` is a no-op).

**Use Cases**:
- Drive colors for `SoftCircle`, `SandLine`, `ParticleSet`, `ParticleField`, `Collage`.
- Provide a stable-yet-reactive “house palette” for an entire patch.
- Provide a weak vector field via `FieldTexture` for subtle motion coupling.

---

## Sink Mods (Mark-Making)

Sink Mods create visual marks on drawing layers. They can be categorized by their visual characteristics.

**Layer pause behavior**: When a drawing layer is paused, Mods targeting that layer must treat it as inactive. While inactive, Mods must drop all incoming sink data for that layer (i.e., do not buffer/accumulate events to be applied later when the layer is unpaused). Layer alpha does not affect whether a layer is considered inactive.

### Soft/Organic Marks

#### SoftCircleMod

Draws soft circles with optional velocity-shaped “brush stroke” behavior.

**Visual Characteristics**: Soft, glowing, ethereal • Organic edges • Can become stroke-like with velocity • Overlap creates luminosity

**Sinks**:
- `Point` (vec2): Circle center positions (normalized 0–1).
- `PointVelocity` (vec4): `{x, y, dx, dy}` stamp with velocity (normalized). Useful when you already have motion vectors and want stable direction.
- `Radius` (float): Base circle size (normalized).
- `Colour` (vec4): Base RGBA color.
- `ColourMultiplier` (float): RGB intensity multiplier.
- `AlphaMultiplier` (float): Alpha multiplier.
- `Softness` (float): Edge falloff amount.
- `EdgeMod` (float): Live edge driver (0–1) used by the edge modulation system.
- `ChangeKeyColour` (float): When > 0.5, cycles through `KeyColours`.

**Key Parameters (core look)**:
- `Radius`: Base size (normalized; converted to pixels using FBO width).
- `Softness`: Blur / edge rolloff.
- `Falloff`: 0=Glow (halo-ish), 1=Dab (premultiplied, less halo).
- `Colour`, `ColourMultiplier`, `AlphaMultiplier`.
- `AgencyFactor`: Auto takeover scaling (multiplies Synth Agency).

**Velocity / stroke shaping**:
- `UseVelocity` (0/1): Enable velocity-based ellipse stretching.
- `VelocitySpeedMin`, `VelocitySpeedMax`: Defines the speed range that maps to 0–1 stretch.
- `VelocityStretch`: How much the stamp elongates at max speed.
- `VelocityAngleInfluence`: 0 = ignore direction, 1 = align major axis to direction.
- `PreserveArea` (0/1): If on, stretching keeps area roughly constant (longer but thinner).
- `MaxAspect`: Clamp maximum elongation.

Notes:
- If you use `Point` only, velocity is inferred from successive points.
- If you provide `PointVelocity`, the `{dx,dy}` is used directly.

**Curvature-driven modulation**:
Curvature is estimated from changes in direction (based on recent point history).
- `CurvatureAlphaBoost`: Boost alpha on high-curvature motion.
- `CurvatureRadiusBoost`: Boost radius on high-curvature motion.
- `CurvatureSmoothing`: Higher = smoother/laggier curvature.

**Edge modulation**:
Edge “waviness” amount is:
- `edgeAmount = clamp(EdgeAmount + EdgeAmountFromDriver * EdgeMod, 0..1)`

Parameters:
- `EdgeAmount`: Base edge modulation.
- `EdgeAmountFromDriver`: How strongly `EdgeMod` drives the edge.
- `EdgeSharpness`, `EdgeFreq1/2/3`.
- `EdgePhaseFromVelocity`: If on, edge pattern rotates with motion direction.

**Coordination rules**:
- If `UseVelocity=1` but you don’t see stretching: increase `VelocityStretch`, and tune `VelocitySpeedMin/Max` to the actual speed scale of your point source.
- If stamps “explode” into long streaks: reduce `VelocityStretch` and/or lower `MaxAspect`.
- If edge modulation does nothing: ensure `EdgeAmountFromDriver > 0` and drive `EdgeMod` (e.g. from `Audio.SpectralCrestScalar`).

**Intent Integration**: Full Intent support with agency.

**Use Cases**:
- Soft particles / mist / glows.
- Motion-reactive brush strokes (feed `PointVelocity` from video flow or particle velocities).
- Audio-driven shimmering edges (drive `EdgeMod` from spectral features).

---

#### ParticleSetMod

Physics-based particle system with attraction, connections, and trails.

**Visual Characteristics**: Organic motion • Dynamic • Connective lines • Physics-driven behavior

**Layer guidance**: When using connection lines (default strategy), treat this as hard-edged geometry and prefer a `7200×7200` drawing layer to avoid aliasing. You can tune line distance via `connectionRadius`; beyond the connection radius it renders points.

**Sinks**:
- `Point` (vec2): Particle spawn position. A small random velocity is added automatically.
- `PointVelocity` (vec4): `{x, y, dx, dy}` spawn with explicit initial velocity.
- `Spin` (float): Rotation speed.
- `Colour` (vec4): Particle color.
- `AlphaMultiplier` (float): Extra alpha scaling for **new** particles (useful when persistence changes).
- `ChangeKeyColour` (float): When > 0.5, cycles through `KeyColours`.

**Key Parameters** (from wrapped engine + wrapper params):
- `Spin`, `Colour`, `AlphaMultiplier`.
- Physics:
  - `timeStep`: Simulation speed.
  - `velocityDamping`: Friction.
  - `attractionStrength`, `attractionRadius`: Inter-particle coupling.
  - `forceScale`: Global force multiplier.
  - `connectionRadius`: Distance threshold for drawing connection lines.
  - `colourMultiplier`: Multiplies particle RGB.
  - `maxSpeed`: Velocity cap.
- `AgencyFactor`: Auto takeover scaling (multiplies Synth Agency).

**Coordination rules**:
- If particles jitter/“explode”: increase `velocityDamping`, decrease `forceScale`/`attractionStrength`, and/or decrease `timeStep`.
- If connections look too busy: lower `connectionRadius` or reduce particle count upstream.
- If you fade the layer faster but want particles to stay visible: increase `AlphaMultiplier` (it only affects new particles, not the layer fade).

**Intent Integration**: Full Intent with agency.

**Use Cases**:
- Flowing particle clouds.
- Audio/video-driven motion when fed via `PointVelocity`.
- Network visualizations (connections).

---

#### ParticleFieldMod

Particle system driven by two texture-based vector fields (often fluid velocity textures).

**Visual Characteristics**: Field-guided motion • Smooth, flowing trajectories • Complex patterns

**Sinks**:
- `Field1Texture` (texture): Primary vector field (RG channels).
- `Field2Texture` (texture): Secondary vector field.
- `PointColour` (vec4): Target color for particles; applied gradually (more aggressively as agency increases).
- `minWeight` (float): Minimum blend weight for Field1 influence.
- `maxWeight` (float): Maximum blend weight for Field1 influence.
- `ChangeKeyColour` (float): When > 0.5, cycles through `KeyColours`.

**Key Parameters** (from wrapped engine + wrapper params):
- Particle population + motion:
  - `ln2ParticleCount`: Log2 particle count control (small changes have large effect).
  - `velocityDamping`: Friction.
  - `forceMultiplier`: Field force scaling (fine control).
  - `maxVelocity`: Speed cap.
  - `particleSize`: Render size.
  - `jitterStrength`, `jitterSmoothing`: Adds organic noise and smooths it.
  - `speedThreshold`: Below this speed, motion is damped/treated as stationary (stabilizer).
- Field coupling:
  - `field1Multiplier`, `field2Multiplier`: Base field strengths.
  - `Field1PreScaleExp`, `Field2PreScaleExp`: Log10 normalization for incoming fields.
    - effective multiplier = `fieldMultiplier * 10^PreScaleExp`
- `AgencyFactor`: Auto takeover scaling (multiplies Synth Agency).

**Coordination rules**:
- If particles barely move: increase `field1Multiplier` first; if still weak, increase `Field1PreScaleExp` by +1.
- If particles “teleport” or streak too hard: decrease `field*Multiplier` and/or decrease `Field*PreScaleExp`.
- If motion is chaotic: increase `velocityDamping`, lower `forceMultiplier`, and consider increasing `speedThreshold`.
- If recoloring is too slow/fast: adjust Synth Agency or feed `PointColour` more/less frequently.

**Intent Integration**: Full Intent with agency (includes particle count, motion, and color).

**Use Cases**:
- Fluid-driven particle motion: `Fluid.velocitiesTexture → ParticleField.Field1Texture`.
- Two-field blends: use palette `FieldTexture` or video flow as the secondary field.
- Large-scale motion texture without explicit point spawning.

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

Divides the drawing space with constrained (minor) and unconstrained (major) divider lines.

Conceptually it is two related systems:
- **Minor/constrained lines**: added incrementally from points or paths and then drawn into the *default* layer.
- **Major/unconstrained lines**: maintained as a small set of “big” dividers that update smoothly from a batch of anchor points, drawn into the `major-lines` layer.

**Visual Characteristics**: Geometric • Sharp edges • Architectural divisions • Optional refraction / chromatic styles for major lines

**Sinks**:
- Minor line inputs:
  - `MinorAnchor` (vec2): Adds an anchor point for constrained line generation.
  - `MinorPath` (ofPath): Converts the path outline into point pairs and adds constrained lines immediately.
- Major line inputs:
  - `MajorAnchor` (vec2): Adds an anchor point for unconstrained major line updates (expects a batch, e.g. cluster centers).
- Color inputs:
  - `MinorLineColour` (vec4): Minor line color.
  - `MajorLineColour` (vec4): Major line color.
  - `ChangeMajorKeyColour` (float): When > 0.5, cycles `MajorKeyColours`.
  - `ChangeMinorKeyColour` (float): When > 0.5, cycles `MinorKeyColours`.
- Control:
  - `ChangeAngle` (float): Updates `Angle` when value > ~0.4 (historical threshold).
  - `ChangeStrategy` (float): When > 0.5, cycles `Strategy` (cooldown ~5s).
  - `ChangeLayer` (float): Standard sink (switch drawing layer assignment).

**Layers**:
- Default layer: constrained/minor lines (`MinorAnchor` / `MinorPath`).
- `major-lines` layer: unconstrained/major lines.
  - If the `major-lines` target layer is an **overlay** (`isOverlay: true`), major lines draw with access to the composite background (required for refraction-style line modes).
  - If the `major-lines` target layer is **not** an overlay, major lines are drawn without background (only Solid/Glow-style major line modes).

**Key Parameters** (wrapper-level):
- Minor line generation:
  - `Strategy`:
    - `0`: point pairs (consume `MinorAnchor` in pairs)
    - `1`: point angles (draw segment from each `MinorAnchor` at `Angle`)
    - `2`: radiating (requires ~7 points; last is centre)
  - `Angle` (0.0–0.5, in π radians): used by Strategy 1.
  - `PathWidth` (0–0.005): width used when converting `MinorPath` outlines.
- Major line rendering:
  - `MajorLineWidth` (pixels): stroke width for major lines.
  - `MaxUnconstrainedLines`: cap on the number of major dividers.
- Colors:
  - `MinorLineColour`, `MajorLineColour`
  - `MinorKeyColours`, `MajorKeyColours` (pipe-separated RGBA list)
- `AgencyFactor`: Auto takeover scaling (multiplies Synth Agency).

**Key Parameters** (from wrapped `DividedArea` engine)
- `unconstrainedSmoothness` (0–1): damping/smoothness for major line motion.
- `minRefPointDistance`, `majorLineStyle`, and other engine controls appear flattened into the Mod’s parameter group.

**Coordination rules**:
- Major anchors are assumed to arrive as a coherent batch (e.g. `Cluster.ClusterCentre` outputs). If your source emits sporadically, major lines will feel unstable.
- If your `major-lines` layer is not overlay, refraction/chromatic line styles cannot work (no background texture available).
- If constrained lines “pile up” too fast: reduce upstream point density or add a slow `Fade` on the default layer.

**Wiring patterns**:
- Audio → geometric architecture:
  - `Audio.PolarPitchRmsPoint → Cluster.Vec2 → DividedArea.MajorAnchor`
- Paths → constrained divisions:
  - `Path.Path → DividedArea.MinorPath`

**Intent Integration**: Responds to Chaos, Granularity, Energy, Density, and Structure. Structure strongly affects major line smoothness (high structure = calmer motion).

**Use Cases**:
- Create architectural divisions driven by audio or motion clusters.
- Combine stable “big structure” (major lines) with busy detail (minor lines).

---

#### CollageMod

Composites a snapshot texture within a path boundary (with optional outline layer).

Collage is **event-driven**: it draws only when it has both a valid `Path` and (for snapshot strategies) an allocated `SnapshotTexture`.
After drawing it clears its stored path/texture and waits for the next pair.

**Visual Characteristics**: Hard-edged shapes • Collage aesthetic • Textural fills • Optional outlines

**Sinks**:
- `Path` (ofPath): Shape boundary. Typically from `PathMod.Path`.
- `SnapshotTexture` (texture): Image to collage. Typically from `PixelSnapshotMod.SnapshotTexture`.
- `Colour` (vec4): Tint color (used when strategy is tinted).
- `OutlineColour` (vec4): Outline stroke color (used when `OutlineAlphaFactor > 0` and an `outlines` layer exists).
- `ChangeKeyColour` (float): When > 0.5, cycles through `KeyColours`.

**Layers**:
- Default layer: filled shape / snapshot.
  - For `Strategy` 1 or 2 (snapshot-based), the default layer **must have a stencil buffer** (`useStencil: true` on the drawing layer). If not, Collage logs an error and does nothing.
- `outlines` layer (optional): outline strokes.
  - Outlines are drawn by “punching” the path interior to transparent and then drawing an outside-aligned stroke.
  - This layer is usually persistent (`clearOnUpdate=false`) and faded slowly.

**Key Parameters**:
- Composition:
  - `Strategy`:
    - `0`: tint fill (no snapshot required)
    - `1`: draw snapshot tinted (stenciled)
    - `2`: draw snapshot untinted (stenciled)
  - `BlendMode`: 0=ALPHA, 1=SCREEN, 2=ADD, 3=MULTIPLY, 4=SUBTRACT.
  - `Opacity`: multiplies tint alpha before drawing (useful to tame SCREEN/ADD blowout).
  - `MinDrawInterval`: rate limiting (seconds). If a burst of paths arrives, this throttles draws.
- Tinting:
  - `Colour`: base tint.
  - `Saturation`: multiplies the tint’s saturation (0–4).
- Outlines:
  - `OutlineAlphaFactor`: 0 disables outlines.
  - `OutlineWidth` (pixels)
  - `OutlineColour`
- `KeyColours`: pipe-separated RGBA list for palette stepping.
- `AgencyFactor`: Auto takeover scaling (multiplies Synth Agency).

**Coordination rules**:
- Snapshot strategies (1/2) require: `useStencil: true` on the target drawing layer.
- If nothing draws: confirm the `Path` is non-trivial (has enough commands) and a fresh snapshot has arrived.
- If tint-fill looks washed out under SCREEN: reduce `Opacity` and/or switch `BlendMode` to ALPHA.
- If you want outlines to linger: keep the outlines layer persistent and add a slow `Fade` on that layer.

**Wiring patterns**:
- Classic collage:
  - `Path.Path → Collage.Path`
  - `PixelSnapshot.SnapshotTexture → Collage.SnapshotTexture`
  - `SomPalette.Random → Collage.Colour`
- Outline persistence:
  - `FadeAlphaMap.float → FadeOutlines.HalfLifeSec` (or a fixed `Fade` preset)

**Intent Integration**: Drives tint color, saturation, outline alpha/width/color for contrast and structure.

**Use Cases**:
- Cut-paper collage effects.
- Sampling and re-projecting a “memory” texture bank into new shapes.
- Texture fills for geometric architecture.

---

### Text Marks

#### TextMod

Draws text by spawning time-based "draw events".

Each received `Text` creates a new draw event that snapshots the current text, colour, position, and font size, then repeatedly draws into the assigned drawing layer for `DrawDurationSec`, fading in with smoothstep easing. Multiple events can overlap (piled-on) up to `MaxDrawEvents`.

For accumulating layers (`clearOnUpdate=false`), TextMod uses incremental alpha so the fade-in stays stable over time instead of building up too quickly. For clearing layers (`clearOnUpdate=true`), it draws the absolute eased alpha each frame.

**Visual Characteristics**: Typographic • Clear letterforms • Positioned text • Fade-in transitions

**Sinks**:
- `Text` (string): Starts a new draw event
- `Position` (vec2): Text position (normalized 0-1, used for newly created events)
- `FontSize` (float): Size multiplier (used for newly created events)
- `Colour` (vec4): Text color (used for newly created events)
- `Alpha` (float): Base opacity multiplier (used for newly created events)
- `DrawDurationSec` (float): Duration of new draw events (seconds)
- `AlphaFactor` (float): Opacity contribution of new draw events (0-1)

**Key Parameters**:
- `DrawDurationSec`: Draw-event duration (0.1-10s)
- `AlphaFactor`: Draw-event opacity contribution (0.0-1.0)
- `MaxDrawEvents`: Maximum concurrent draw events (oldest dropped)
- `MinFontPx`: Minimum font size in pixels
- `AgencyFactor`: Auto takeover scaling (multiplies Synth Agency)

**Intent Integration**: Full Intent support with agency (including draw-event envelope).

**Use Cases**:
- Display lyrics synchronized to music
- Create typographic compositions
- Add textual annotations to visuals

---

### Fluid/Dynamic Marks

#### FluidRadialImpulseMod

Adds energy to a `FluidMod` velocity layer via radial, directional, and swirl impulses.

**Visual Characteristics**: Fluid distortion • Ripples • Directional flow injection • Vortex seeds

**Sinks**:
- `Point` (vec2): Impulse centers (normalized 0–1).
- `PointVelocity` (vec4): `{x, y, dx, dy}` (normalized) per-impulse directional injection.
- `Velocity` (vec2): Global `{dx, dy}` used when only `Point` is provided.
- `SwirlVelocity` (float): Additional swirl term (overrides config value).
- `Impulse Radius` (float): Impulse size.
- `Impulse Strength` (float): Impulse intensity.

**Key Parameters**:
- `Impulse Radius` (0.0–0.10): Radius as a fraction of the target buffer’s min dimension.
- `Impulse Strength` (0.0–0.7): Interpreted as a **fraction of radius displacement per step** (resolution-independent feel).
- `VelocityScale` (0.0–50.0): Scales normalized `Velocity` / `PointVelocity` into pixel displacement per step.
- `dt` (0.0–0.002): dt passed to the impulse shader (should usually match the fluid solver’s dt semantics).
- `SwirlVelocity` (0.0–0.1): Base swirl term.
- `SwirlStrength` (0.0–0.2): Multiplier for swirl (capped to avoid obvious whirlpools).

**Coordination rules**:
- If the impulse does nothing: increase `Impulse Strength` first; if directional impulses are weak, increase `VelocityScale`.
- If the fluid “blows out” instantly: lower `Impulse Strength`, and/or lower `Fluid.dt`/increase `Fluid.VelocityDissipation`.
- If it looks too laminar: add a little swirl (small `SwirlVelocity` + `SwirlStrength`).

**Intent Integration**: Responds to Intent (radius + strength). Other parameters are manual-only.

**Use Cases**:
- Inject flow from video motion: `VideoFlowSource.PointVelocity → FluidRadialImpulse.PointVelocity`.
- Audio-driven ripples: drive `Point` from audio clusters; drive `Impulse Strength` from a scalar.
- Seed turbulence into a motion field used downstream (`Smear`, `ParticleField`).

---

### Debug/Visualization

#### IntrospectorMod

Visualizes data points and horizontal lines for debugging.

**Visual Characteristics**: Debug overlay • Non-persistent (separate FBO) • Colored traces

**Sinks**:
- `Point` (vec2): Points to visualize
- `HorizontalLine1`, `HorizontalLine2`, `HorizontalLine3` (float): Y-positions for lines

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

**Visual Characteristics**: Flowing • Organic motion • Vortex formation • Smooth blending • Plumes (buoyancy)

**Sources**:
- `velocitiesTexture` (texture): Velocity field texture

**Sinks** (temperature injection):
- `TempImpulsePoint` (vec2): Normalized 0–1 injection centers (queued and applied each frame).
- `TempImpulseRadius` (float): Overrides `TempImpulseRadius` parameter via controller.
- `TempImpulseDelta` (float): Overrides `TempImpulseDelta` parameter via controller.

**Sinks** (velocity field injection):
- `VelocityFieldTexture` (texture): External velocity field (sampled in normalized coords and added into the solver’s velocity buffer).
- `VelocityFieldPreScaleExp` (float): Log10 normalization exponent (effective preScale = `10^exp`).
- `VelocityFieldMultiplier` (float): Fine multiplier. Set to 0 to disable.

Velocity field scaling rule:
- effectiveScale = `(10^VelocityFieldPreScaleExp) * VelocityFieldMultiplier`

**Layers**:
- Default layer: Visual simulation output
- `velocities` layer: Velocity field (can drive ParticleFieldMod)
- `obstacles` layer (optional): Obstacle mask sampled by the fluid solver

**Key Parameters**:
- `Boundary Mode`: 0=SolidWalls, 1=Wrap, 2=Open (must match GL wrap).
- `dt`: Time step (simulation speed).
- `Vorticity`: Vortex formation strength.
- `Value Dissipation`: Dye persistence (normalized 0–1).
- `Velocity Dissipation`: Motion persistence (normalized 0–1).
- `Value Spread`: Dye diffusion (normalized 0–1).
- `Velocity Spread`: Velocity diffusion/viscosity (normalized 0–1).
- `Value Max`: Clamp after advection (0 = disabled).
- Velocity field injection:
  - `VelocityFieldPreScaleExp` + `VelocityFieldMultiplier`: External flow strength (effectiveScale = `10^exp * mult`).
  - Use `VelocityFieldMultiplier = 0` to disable without disconnecting.

Tuning hints:
- If motion decays too quickly: increase `Velocity Dissipation` and/or increase `dt` slightly.
- If motion is too smeary/viscous: decrease `Velocity Spread`.
- If the whole sim explodes: reduce `dt` and/or increase dissipation.

**Temperature (v2 scalar field)**:
- `TempEnabled`: Enables temperature advection/diffusion
- `Temperature Dissipation`, `Temperature Spread`, `Temperature Iterations`: Temperature evolution controls
- `TempImpulseRadius`: Default injection radius (normalized 0–0.10)
- `TempImpulseDelta`: Default injected delta (-1.0–1.0)

**Obstacles** (optional):
- `ObstaclesEnabled`: Enables obstacle masking (requires `layers.obstacles`)
- `Obstacle Threshold`: Threshold for `max(alpha, luma(rgb))`
- `Obstacle Invert`: Invert mask
- Requirements: obstacle layer must match velocity resolution and boundary-mode wrap (hard error if mismatched)

**Buoyancy**:
- **v1 (dye-driven)**: set `Buoyancy Strength` > 0 and keep `Use Temperature` off
  - `Buoyancy Density Scale`, `Buoyancy Threshold`, `Gravity Force X/Y`
- **v2 (temperature-driven)**: set `Use Temperature` on + configure temperature
  - `Ambient Temperature`, `Temperature Threshold`, `Gravity Force X/Y`

- `AgencyFactor`: Auto takeover scaling (multiplies Synth Agency)

**Intent Integration**: Core sim parameters support Intent with agency; temperature and buoyancy parameters are currently manual-only.

**Use Cases**:
- Create flowing organic backgrounds
- Add fluid motion to particle systems
- Create “invisible forces” (temperature-driven buoyancy without adding dye)
- Add plumes/smoke behavior to marks (dye-driven buoyancy)

---

### FadeMod

Fades a drawing layer over time using a time-based half-life model.

Internally it derives a per-frame multiplier from `HalfLifeSec` and the current frame `dt` so the fade rate stays stable across frame rates.

**Visual Characteristics**: Gradual fade • Trails • Temporal blending

**Sinks**:
- `HalfLifeSec` (float): Preferred input. Time (seconds) for the layer to reach 50% intensity.
- `Alpha` (float, legacy): Interpreted as **alpha-per-frame at 30fps** and converted to an equivalent `HalfLifeSec`.

**Key Parameters**:
- `HalfLifeSec` (0.05–300): Larger = more persistent trails.
- `AgencyFactor`: Auto takeover scaling (multiplies Synth Agency).

**Intent Integration**: Responds to Intent (mapped onto `HalfLifeSec`, with agency).

**Use Cases**:
- Create trailing effects on accumulating layers (`clearOnUpdate=false`)
- Slowly clear the canvas without hard resets
- Make event-driven marks linger (e.g. Collage outlines)

---

### SmearMod

Applies displacement-based smearing using vector fields with optional spatial "teleport" strategies.

Smear is a feedback effect: it samples the previous frame, displaces it, and blends it with the current frame.

**Visual Characteristics**: Smeared/distorted • Echo effects • Displacement patterns • Glitchy artifacts (depending on strategy)

**Sinks**:
- `Translation` (vec2): Direct translation offset (normalized).
- `MixNew` (float): Blend weight for *new* pixels (0.3–1.0). Lower = more feedback.
- `HalfLifeSec` (float): Time-based trail persistence (preferred).
- `AlphaMultiplier` (float, legacy): Interpreted as per-frame multiplier at 30fps and converted to `HalfLifeSec`.
- `Field1Multiplier` (float): Primary field strength (pre-scale is applied separately).
- `Field2Multiplier` (float): Secondary field strength.
- `Field1Texture` (texture): Primary displacement field (RG channels).
- `Field2Texture` (texture): Secondary displacement field.
- `ChangeLayer` (float): Control command (change/disable/reset drawing layer depending on value).

**Key Parameters**:
- `MixNew`: Lower = stronger feedback, higher = cleaner.
- `HalfLifeSec`: Lower = faster decay, higher = longer trails.
- `Translation`: Constant drift.
- `Field1PreScaleExp`, `Field2PreScaleExp`: Log10 exponent; effective field scale is `10^exp`.
- `Field1Multiplier`, `Field1Bias`: Primary field control.
- `Field2Multiplier`, `Field2Bias`: Secondary field control.
- `Strategy` (0–9): Spatial manipulation mode (teleports/folds).
- `GridSize` and strategy-specific parameters (`JumpAmount2`, `BorderWidth7`, `GridLevels5`, `GhostBlend8`, `FoldPeriod9`).

**Intent Integration**: Responds to Intent (mix, half-life, and some strategy parameters via controllers).

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
AudioDataSourceMod (Synth audio input)
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
AudioDataSourceMod (Synth audio input)
  └─ Onset1 → RandomVecSourceMod.trigger

RandomVecSourceMod
  └─ Vec2 → ParticleSetMod.Point

FluidMod (generates velocity field)
  └─ velocitiesTexture → ParticleFieldMod.Field1Texture

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
  └─ ClusterCentre → DividedAreaMod.MajorAnchor

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
  └─ PitchRmsPoint → SoftCircleMod.Point
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
  └─ PointVelocity → FluidRadialImpulseMod.PointVelocity (optional: inject camera flow into fluid)
  └─ FlowField → SmearMod.Field1Texture

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
  └─ Vec2 → FluidRadialImpulseMod.Point

FluidRadialImpulseMod (adds energy to fluid)

FluidMod (creates flowing patterns)
  └─ velocitiesTexture → SmearMod.Field1Texture

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
  └─ ClusterCentre → DividedAreaMod.MinorAnchor

AudioDataSourceMod
  └─ Onset1 → ClusterMod.ChangeClusterCount

DividedAreaMod (Strategy: radiating)

PixelSnapshotMod
  └─ SnapshotTexture → CollageMod.SnapshotTexture (textured fills behind DividedArea lines)

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

Many Mods support the **Intent System** for expressive real-time control. The Intent system maps external controllers to five abstract dimensions (all normalized 0.0-1.0):

- **Energy**: Speed, motion, intensity, magnitude
- **Density**: Quantity, opacity, detail level
- **Structure**: Organization, brightness, pattern regularity
- **Chaos**: Randomness, variance, noise
- **Granularity**: Scale, resolution, feature size

Mods with `AgencyFactor` parameters can scale their effective Synth Agency for auto-wired parameters (i.e. how much connected signals take over as you raise Synth Agency). Intent influence is controlled by the Intent faders/strength and each Mod’s `applyIntent()` mapping; it is not currently scaled by `AgencyFactor`.

For detailed Intent mappings per Mod, see [intent-mappings.md](intent-mappings.md).

---

---

## Synth (Core)

The Synth itself provides sources and sinks that can be used in the connection graph.

### Existing Sources

- `CompositeFbo` (ofFbo): The final composited drawing (all layers combined)

### Memory Bank

The Synth maintains a **Memory Bank** of 8 texture slots that store random crops captured from the full-resolution composite. These "memories" can be accumulated during a live performance and recalled later, bringing fine details that are normally invisible (due to downscaling for display) back into the visual mix.

**Memory Bank Sources**:
- `Memory` (ofTexture): Emits memory texture (selection depends on which sink triggered the emit)

**Memory Bank Sinks**:

*Save Operations*:
- `MemorySave` (float): Trigger (>0.5) saves a crop using centre/width parameters
- `MemorySaveSlot` (float): Value 0-7 saves to specific slot

*Emit Operations*:
- `MemoryEmit` (float): Trigger (>0.5) emits using centre/width parameters
- `MemoryEmitSlot` (float): Value 0-7 emits from specific slot
- `MemoryEmitRandom` (float): Trigger emits random memory
- `MemoryEmitRandomNew` (float): Trigger emits with recency weighting
- `MemoryEmitRandomOld` (float): Trigger emits with old-memory weighting

*Parameter Updates*:
- `MemorySaveCentre` (float): Legacy/manual overwrite selection for `MemorySave` (auto-capture ignores)
- `MemorySaveWidth` (float): Legacy/manual overwrite spread for `MemorySave` (auto-capture ignores)
- `MemoryEmitCentre` (float): Centre of emit slot selection (0-1)
- `MemoryEmitWidth` (float): Width/spread of emit slot selection (0-1)

*Management*:
- `MemoryClearAll` (float): Trigger (>0.5) clears all memory slots

**Intent Integration**:
- Chaos → MemoryEmitWidth (more chaos = wider/more random selection)
- Energy → MemoryEmitCentre (high energy = recent memories)
- Structure → MemorySaveWidth (more structure = more predictable saves)

**GUI**: The Memory Bank section shows thumbnails of all 8 slots with manual Save buttons.

**Auto Capture (Performance Safety Net)**:
- Enabled by default via `MemoryAutoCaptureEnabled`
- Warmup: fills all 8 slots quickly (target `MemoryAutoCaptureWarmupTargetSec` = 120s)
- After warmup: maintains time-banded coverage across the whole performance (slots 0-2 refresh ~10min, 6-7 refresh ~15s by default)
- Uses a tiny downsampled density check to avoid saving mostly-empty frames; warmup thresholds relax over ~120s to guarantee the bank fills
- Most performance configs should not need explicit `MemorySave` triggers when auto-capture is enabled (prefer keeping Memory wiring minimal and using `MemoryEmit*` to recall)

**Auto Capture Parameters**:
- `MemoryAutoCaptureEnabled` (bool)
- `MemoryAutoCaptureWarmupTargetSec` (float)
- `MemoryAutoCaptureWarmupIntervalSec` (float)
- `MemoryAutoCaptureRetryIntervalSec` (float)
- `MemoryAutoCaptureRecentIntervalSec` (float)
- `MemoryAutoCaptureMidIntervalSec` (float)
- `MemoryAutoCaptureLongIntervalSec` (float)
- `MemoryAutoCaptureMinVariance` (float)
- `MemoryAutoCaptureMinActiveFraction` (float)
- `MemoryAutoCaptureWarmupBurstCount` (int)
- `MemoryAutoCaptureAnalysisSize` (int)

**Example Connections**:
```
# Optional: manual/forced save (usually unnecessary when auto-capture is enabled)
# Timer.Tick -> Synth.MemorySave

# Emit random memory on audio onset
AudioData.Onset -> Synth.MemoryEmitRandom

# Connect emitted memory to CollageMod
Synth.Memory -> CollageA.SnapshotTexture
# Optional second collage voice
Synth.Memory -> CollageB.SnapshotTexture
```

**Use Cases**:
- Recall earlier visual states during improvisation
- Surface fine details from the high-res composite
- Create temporal layering in performances
- Build visual "memory palace" compositions

---

**Document Version**: 1.4
**Last Updated**: December 8, 2025
