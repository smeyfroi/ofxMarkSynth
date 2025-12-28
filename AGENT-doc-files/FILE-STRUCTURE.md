# Project File Structure

## Documentation

```
AGENT-doc-files/
├── docs/
│   ├── AI-Assistant/
│   │   ├── Status/
│   │   │   ├── CURRENT-TASKS.md
│   │   │   ├── KNOWN-BUGS.md
│   │   │   └── TODO.md
│   │   ├── AGENTS.md
│   │   ├── CURRENT-STATE.md
│   │   ├── KEYBOARD-SHORTCUTS.md
│   │   ├── PROJECT-CONTEXT.md
│   │   ├── RULES.md
│   │   └── hierarchical-layout-summary.md
│   └── CHANGELOG.md
├── AI-AGENTS-GUIDE.md
├── FILE-STRUCTURE.md
└── generate-FILE-STRUCTURE.py
```

## Source Code Structure

The `src/` directory is organized into categorical subdirectories:

```
src/
├── config/              # Configuration and serialization (5 files)
│   ├── ModFactory           # Mod type registration and instantiation
│   ├── ModSnapshotManager   # Saving/restoring Mod parameter snapshots
│   ├── Parameter            # Parameter wrapper utilities
│   ├── PerformanceNavigator # Config navigation and playlist management
│   └── SynthConfigSerializer # JSON config loading/saving
│
├── controller/          # Runtime controllers (7 files)
│   ├── ConfigTransitionManager  # Smooth transitions between configs
│   ├── DisplayController        # Display/window management
│   ├── HibernationController    # Power/idle state management
│   ├── IntentController         # Intent blending and control
│   ├── LayerController          # Drawing layer management
│   ├── MemoryBankController     # Memory bank slot management
│   └── TimeTracker              # Frame timing and performance tracking
│
├── core/                # Core framework classes (7 files)
│   ├── Gui                  # Main ImGui interface
│   ├── Intent               # Artistic intent definition
│   ├── IntentMapper         # Intent name/ID mapping
│   ├── IntentMapping        # Intent parameter mappings
│   ├── MemoryBank           # Texture memory storage
│   ├── Mod                  # Base module class
│   ├── ParamController      # Parameter automation controller
│   ├── Synth                # Main synthesizer orchestrator
│   └── SynthConstants.h     # Core constants (MOD_ID_START, etc.)
│
├── gui/                 # GUI utilities (4 files)
│   ├── FontCache            # Font loading and caching
│   ├── GuiConstants.h       # GUI dimension constants
│   ├── HelpContent          # Help text definitions
│   ├── ImGuiUtil            # ImGui helper functions
│   └── LoggerChannel        # Custom ofLog channel for GUI
│
├── layerMods/           # Layer effect modules (3 files)
│   ├── FadeMod              # Fade/dissolve effects
│   ├── FluidMod             # Fluid simulation layer
│   └── SmearMod             # Smear/blur effects
│
├── nodeEditor/          # Node graph editor (4 files)
│   ├── NodeEditorLayout           # Force-directed layout algorithm
│   ├── NodeEditorLayoutSerializer # Layout persistence
│   ├── NodeEditorModel            # Node/link data model
│   └── NodeRenderUtil             # Node rendering helpers
│
├── processMods/         # Processing modules (5 files)
│   ├── (Various process mods for data transformation)
│
├── rendering/           # Rendering subsystem (6 files)
│   ├── AsyncImageSaver      # Threaded EXR/image export
│   ├── CompositeRenderer    # Multi-layer compositing with tonemapping
│   ├── FboCopy              # FBO utility operations
│   ├── RenderingConstants.h # Video/rendering constants
│   ├── TonemapCrossfadeShader.h # HDR tonemapping shader
│   └── VideoRecorder        # Video recording controller
│
├── sinkMods/            # Output/rendering modules (9 files)
│   ├── CollageMod           # Image collage rendering
│   ├── DividedAreaMod       # Area subdivision rendering
│   ├── FluidRadialImpulseMod # Fluid impulse effects
│   ├── IntrospectorMod      # Debug visualization
│   ├── ParticleFieldMod     # Particle field rendering
│   ├── ParticleSetMod       # Particle set rendering
│   ├── SandLineMod          # Sand/line drawing
│   ├── SoftCircleMod        # Soft circle rendering
│   └── TextMod              # Text rendering
│
├── sourceMods/          # Input/data source modules (8 files)
│   ├── AudioDataSourceMod   # Audio analysis input
│   ├── RandomFloatSourceMod # Random float generator
│   ├── RandomHslColorMod    # Random color generator
│   ├── RandomVecSourceMod   # Random vector generator
│   ├── StaticTextSourceMod  # Static text source
│   ├── TextSourceMod        # Dynamic text source
│   ├── TimerSourceMod       # Timer-based source
│   └── VideoFlowSourceMod   # Video/optical flow input
│
├── util/                # Small utilities (3 files)
│   ├── Lerp.h               # Interpolation helpers
│   ├── Oklab.h              # Oklab color space utilities
│   └── TimeStringUtil.h     # Time formatting utilities
│
└── ofxMarkSynth.h       # Main addon include header
```

## Example Projects

Each example demonstrates specific functionality:

```
example_simple/          # Basic setup
example_audio/           # Audio reactivity
example_audio_clusters/  # Audio with point clustering
example_audio_clusters_dividedarea/  # Audio + divided area
example_audio_palette/   # Audio with SOM palette
example_collage/         # Image collage
example_fade/            # Fade effects
example_memory_collage/  # Memory bank collage
example_particles/       # Basic particles
example_particles_from_video/ # Video-driven particles
example_points/          # Point rendering
example_sandlines/       # Sand line drawing
example_smear/           # Smear effects
example_text/            # Text rendering
example_timer_text/      # Timer-driven text
```

## Key Dependencies

External addons used by ofxMarkSynth:
- `ofxIntrospector` - Debug visualization
- `ofxGui` - Parameter GUI panels
- `ofxAudioData` - Audio analysis
- `ofxRenderer` - Rendering utilities
- `ofxDividedArea` - Area subdivision
- `ofxMotionFromVideo` - Video motion extraction
- `ofxParticleSet` - Particle systems
- `ofxPlottable` - Plotting utilities
- `ofxPointClusters` - Point clustering
- `ofxSomPalette` - Self-organizing map palettes
- `ofxConvexHull` - Convex hull computation
- `ofxTimeMeasurements` - Performance profiling
