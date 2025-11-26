# SYNTH-RESOURCES.md — ofxMarkSynth Resource Manager Guide

This document lists all resources that Mods require via the `ResourceManager` and shows how to provide them.

## Overview

`ResourceManager` is a small, type-safe container passed to `Synth` and used by `ModFactory` when constructing Mods. Add resources before creating the `Synth` (or before `loadFromConfig()`), and reference them by name and type.

- Add: `resources.add("name", value)`
- Get (internal): `resources.get<T>("name")`
- Types must match exactly (e.g., `std::filesystem::path`, `glm::vec2`, `bool`).

## Resource Requirements by Mod

### AudioDataSource
Provides audio features from a file or microphone.

Choose one configuration:
- File playback:
  - `sourceAudioPath` (std::filesystem::path)
  - `audioOutDeviceName` (std::string)
  - `audioBufferSize` (int)
  - `audioChannels` (int)
  - `audioSampleRate` (int)
- Microphone input:
  - `micDeviceName` (std::string)
  - `recordAudio` (bool)
  - `audioRecordingPath` (std::filesystem::path)

Example:
```cpp
ofxMarkSynth::ResourceManager resources;
// File playback (all audio config required)
resources.add("sourceAudioPath", std::filesystem::path("audio/track.wav"));
resources.add("audioOutDeviceName", std::string("Apple Inc.: MacBook Pro Speakers"));
resources.add("audioBufferSize", 256);
resources.add("audioChannels", 1);
resources.add("audioSampleRate", 48000);

// OR microphone
resources.add("micDeviceName", std::string("Built-in Microphone"));
resources.add("recordAudio", true);
resources.add("audioRecordingPath", std::filesystem::path("recordings/"));
```

---

### VideoFlowSource
Provides optical flow from a video file or camera.

Choose one configuration:
- Video file:
  - `sourceVideoPath` (std::filesystem::path)
  - `sourceVideoMute` (bool)
- Camera input:
  - `cameraDeviceId` (int)
  - `videoSize` (glm::vec2)  // width, height
  - `saveRecording` (bool)
  - `recordingPath` (std::filesystem::path)

Example:
```cpp
ofxMarkSynth::ResourceManager resources;
// Video file
resources.add("sourceVideoPath", std::filesystem::path("video/input.mp4"));
resources.add("sourceVideoMute", false);

// OR camera
resources.add("cameraDeviceId", 0);
resources.add("videoSize", glm::vec2(1280, 720));
resources.add("saveRecording", true);
resources.add("recordingPath", std::filesystem::path("video-recordings/"));
```

---

### Text
Renders incoming strings to a drawing layer.

- `fontPath` (std::filesystem::path)

Example:
```cpp
ofxMarkSynth::ResourceManager resources;
resources.add("fontPath", std::filesystem::path("fonts/Roboto-Regular.ttf"));
```

---

### TextSource
Emits strings loaded from files in a directory.

- `textSourcesPath` (std::string) // base directory for text files

Example:
```cpp
ofxMarkSynth::ResourceManager resources;
resources.add("textSourcesPath", ofToDataPath("text"));
```

---

## Mods That Do Not Require Resources

These Mods are constructed without `ResourceManager` dependencies:

- Source Mods: `StaticTextSource`, `TimerSource`, `RandomFloatSource`, `RandomHslColor`, `RandomVecSource`
- Process Mods: `Cluster`, `Fluid`, `FluidRadialImpulse`, `MultiplyAdd`, `ParticleField`, `ParticleSet`, `Path`, `PixelSnapshot`, `Smear`, `SoftCircle`
- Sink Mods: `Collage`, `DividedArea`, `Introspector`, `SandLine`, `SomPalette`
- Layer Mods: `Fade`

---

## Usage Pattern

```cpp
ofxMarkSynth::ResourceManager resources;
// Add required resources for the Mods you will create
resources.add("fontPath", std::filesystem::path("fonts/Arial.ttf"));
resources.add("sourceAudioPath", std::filesystem::path("audio/music.wav"));

auto synth = std::make_shared<ofxMarkSynth::Synth>(
  "MySynth",
  ofxMarkSynth::ModConfig{},
  /*startPaused*/ false,
  /*compositeSize*/ glm::vec2(1920, 1920),
  resources
);

// If loading from config, make sure resources are set before:
// synth->loadFromConfig(ofToDataPath("configs/my-synth.json"));
```

---

## Notes

- Missing required resources cause Mod creation to fail; errors are logged.
- Use the exact types expected by each Mod.
- Paths should be `std::filesystem::path` for portability.
- `ResourceManager` stores values internally as `std::shared_ptr<T>`; lifetime is managed for you.

## References

- `src/util/ModFactory.hpp` — `ResourceManager` definition
- `src/util/ModFactory.cpp` — Built-in Mod registrations and resource access
- Examples: `example_audio/`, `example_particles_from_video/`, `example_text/`
