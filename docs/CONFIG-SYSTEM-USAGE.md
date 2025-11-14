# Synth Config System - Usage Guide

## Quick Start

### 1. Create a JSON Config

Create a config file at `configs/examples/my-config.json`:

```json
{
  "version": "1.0",
  "name": "my-config",
  "description": "My first config",
  
  "drawingLayers": {
    "background": {
      "size": [1920, 1080],
      "internalFormat": "GL_RGBA",
      "wrap": "GL_CLAMP_TO_EDGE",
      "fadeBy": 0.0,
      "blendMode": "OF_BLENDMODE_ALPHA",
      "useStencil": false,
      "numSamples": 0,
      "isDrawn": true
    }
  },
  
  "mods": {
    "Random Points": {
      "type": "RandomVecSourceMod",
      "config": {
        "CreatedPerUpdate": "0.4",
        "Dimension": "2"
      }
    }
  },
  
  "connections": [
    "Random Points.vec2 -> SomeMod.points"
  ],
  
  "intents": {
    "Calm": {
      "energy": 0.2,
      "density": 0.3,
      "structure": 0.7,
      "chaos": 0.1,
      "granularity": 0.1
    }
  }
}
```

### 2. Load Config in Your App

In your `ofApp.cpp`:

```cpp
#include "ofApp.h"

void ofApp::setup() {
  ofSetBackgroundColor(0);
  
  // Create resources that Mods need
  introspectorPtr = std::make_shared<Introspector>();
  introspectorPtr->visible = true;
  
  // Build resource map for Mods that need external dependencies
  ofxMarkSynth::ResourceMap resources;
  resources["introspector"] = introspectorPtr;
  
  // Load config instead of calling createMods()
  std::string configPath = ofToDataPath("configs/my-config.json");
  synth.loadFromConfig(configPath, resources);
  
  // Configure GUI as normal
  synth.configureGui(ofGetWindowPtr());
}

void ofApp::update() {
  synth.update();
  introspectorPtr->update();
}

void ofApp::draw() {
  synth.draw();
  synth.drawGui();
}
```

### 3. Run Your App

The config will be loaded on startup. Edit the JSON and rerun (no recompilation needed!)

## JSON Format Reference

### Top-Level Structure

```json
{
  "version": "1.0",              // Config format version
  "name": "config-name",         // Short name for this config
  "description": "...",          // Human-readable description
  "compositeSize": [1920, 1080], // Optional: Synth output size
  "startPaused": false,          // Optional: Start paused?
  
  "drawingLayers": { ... },      // Drawing layers (optional)
  "mods": { ... },               // Mods to create (required)
  "connections": [ ... ],        // How Mods connect (optional)
  "intents": { ... }             // Intent presets (optional)
}
```

### Drawing Layers

Name-keyed objects defining FBOs:

```json
"drawingLayers": {
  "LayerName": {
    "size": [1920, 1080],
    "internalFormat": "GL_RGBA32F",  // GL_RGBA, GL_RGB, GL_RGBA32F, GL_RGB32F, etc.
    "wrap": "GL_CLAMP_TO_EDGE",      // GL_CLAMP_TO_EDGE, GL_REPEAT, GL_MIRRORED_REPEAT
    "fadeBy": 0.95,                   // 0.0 = no fade, 1.0 = instant clear
    "blendMode": "OF_BLENDMODE_ADD",  // OF_BLENDMODE_ALPHA, _ADD, _MULTIPLY, etc.
    "useStencil": false,
    "numSamples": 0,                  // MSAA samples (0 = no AA)
    "isDrawn": true                   // Include in composite?
  }
}
```

### Mods

Name-keyed objects defining Mod instances:

```json
"mods": {
  "ModName": {
    "type": "ModClassName",     // Exact C++ class name
    "config": {                 // Initial parameter values
      "ParamName": "value"
    },
    "resources": {              // Optional: external dependencies
      "resourceKey": "ptrName"
    }
  }
}
```

**Common Mod Types:**

**Sources:**
- `RandomVecSourceMod` - Random points/vectors
- `RandomFloatSourceMod` - Random float values
- `RandomHslColorMod` - Random colors
- `AudioDataSourceMod` - Audio analysis (requires `audioDataProcessor` resource)
- `VideoFlowSourceMod` - Optical flow from video

**Processors:**
- `ClusterMod` - Point clustering
- `ParticleSetMod` - Particle system
- `PathMod` - Path from points
- `FluidMod` - Fluid simulation
- `TranslateMod` - Transform/translate
- `SmearMod` - Smear effect
- `SoftCircleMod` - Soft circles

**Sinks (Drawing):**
- `IntrospectorMod` - Debug visualizer (requires `introspector` resource)
- `SandLineMod` - Sand line renderer
- `CollageMod` - Collage renderer
- `SomPaletteMod` - SOM palette
- `DividedAreaMod` - Divided area

**Layers:**
- `FadeMod` - Fade layer over time

**Shaders:**
- `AddTextureMod` - Add texture overlay

### Connections

Array of connection strings using DSL format:

```json
"connections": [
  "SourceMod.outputPort -> SinkMod.inputPort",
  "Mod1.output -> Mod2.input",
  "Mod2.result -> Synth.compositeFbo"
]
```

**Port names** are defined per-Mod type. Check the Mod's source code for available ports.

### Intents

Name-keyed objects defining Intent presets:

```json
"intents": {
  "IntentName": {
    "energy": 0.5,      // 0.0 to 1.0 - motion, speed, activity
    "density": 0.5,     // 0.0 to 1.0 - amount of elements
    "structure": 0.5,   // 0.0 to 1.0 - organization, patterns
    "chaos": 0.5,       // 0.0 to 1.0 - randomness
    "granularity": 0.5  // 0.0 to 1.0 - scale of features
  }
}
```

## Resource Injection

Some Mods need external resources (audio processor, introspector, etc.). Specify these in the JSON:

```json
"mods": {
  "Audio Points": {
    "type": "AudioDataSourceMod",
    "config": {...},
    "resources": {
      "audioDataProcessor": "audioDataProcessorPtr"
    }
  }
}
```

Then provide them in code:

```cpp
// Create the actual resource
audioDataProcessorPtr = std::make_shared<ofxAudioData::Processor>(...);

// Build resource map
ofxMarkSynth::ResourceMap resources;
resources["audioDataProcessor"] = audioDataProcessorPtr;  // Key matches JSON

// Load with resources
synth.loadFromConfig(configPath, resources);
```

**Common Resources:**
- `audioDataProcessor` → `std::shared_ptr<ofxAudioData::Processor>`
- `introspector` → `std::shared_ptr<Introspector>`

## Live Performance Workflow

### Organize Configs by Show Section

```
~/Documents/MarkSynth/configs/winter-concert-2025/
  00-soundcheck.json
  01-quiet-opening.json
  02-building-energy.json
  03-peak-chaos.json
  04-resolution.json
```

### Load Configs Programmatically

```cpp
void ofApp::loadSection(int sectionNum) {
  std::string configName = "0" + ofToString(sectionNum) + "-section.json";
  std::string configPath = ofToDataPath("configs/winter-concert-2025/" + configName);
  
  if (synth.loadFromConfig(configPath, resources)) {
    ofLogNotice() << "Loaded section " << sectionNum;
  }
}

void ofApp::keyPressed(int key) {
  // Load sections with number keys
  if (key >= '0' && key <= '9') {
    int section = key - '0';
    loadSection(section);
  }
}
```

### Three Layers of Control

1. **Pre-performance (JSON config):**
   - Which Mods exist
   - How they're connected
   - Available Intent presets
   - Initial parameter values

2. **Mid-level live (Intent + Agency):**
   - Intent activation sliders (via MIDI controller)
   - Agency slider (auto ↔ manual control)

3. **Fine-grained live (Parameters):**
   - Direct Mod parameter tweaking
   - GUI control during performance

## Tips & Best Practices

### Start Simple

Begin with a minimal config:
```json
{
  "version": "1.0",
  "name": "test",
  "mods": {
    "Points": {
      "type": "RandomVecSourceMod",
      "config": {"CreatedPerUpdate": "0.5", "Dimension": "2"}
    }
  }
}
```

### Test Early

Load the config and verify it works before adding complexity.

### Use Descriptive Names

```json
"mods": {
  "Audio Pitch Points": { ... },      // Good: describes what it is
  "High Frequency Clusters": { ... }, // Good: clear purpose
  "mod1": { ... }                     // Bad: meaningless
}
```

### Comment with Description Field

```json
{
  "name": "energetic-peak",
  "description": "High density, lots of motion, chaotic structure. For the climax section."
}
```

### Version Your Configs

```
configs/winter-concert-2025/
  01-opening-v1.json
  01-opening-v2.json   ← refined version
  01-opening-final.json
```

### Check Logs

The config loader provides detailed logging:
```
[notice ] SynthConfigSerializer: Loading config from: configs/test.json
[notice ] SynthConfigSerializer: Created drawing layer: background
[notice ] SynthConfigSerializer: Created Mod: Random Points (RandomVecSourceMod)
[notice ] SynthConfigSerializer: Parsed 1 connections
[notice ] Synth: Successfully loaded config from: configs/test.json
```

## Troubleshooting

### "Unknown Mod type: ModName"

Check the type name matches exactly (case-sensitive):
- ✅ `"ClusterMod"`
- ❌ `"clustermod"`, `"cluster"`

### "Mod requires 'resourceName' resource"

Add the resource to your ResourceMap:
```cpp
resources["resourceName"] = resourcePtr;
```

### "No mods section in config"

Every config must have a `"mods"` section, even if empty:
```json
{
  "version": "1.0",
  "name": "empty",
  "mods": {}
}
```

### Config file not found

Use `ofToDataPath()` to get absolute path:
```cpp
std::string configPath = ofToDataPath("configs/my-config.json");
```

Or use absolute path:
```cpp
std::string configPath = "/Users/you/Documents/MarkSynth/configs/my-config.json";
```

## Next Steps

1. ✅ Create your first JSON config
2. ✅ Load it in an example app
3. Create 2-3 configs for different moods/sections
4. Map Intent activations to MIDI controller
5. Practice switching between configs during rehearsal
6. Refine configs based on performance needs

Happy performing! 🎵✨
