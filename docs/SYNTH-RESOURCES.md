# SYNTH-RESOURCES.md — ofxMarkSynth Resource Manager Guide

This document lists all resources that `Synth` and Mods require via the `ResourceManager` and shows how to provide them.

## Overview

`ResourceManager` is a small, type-safe container passed to `Synth` and used by `ModFactory` when constructing Mods. Add resources before creating the `Synth` (or before `loadFromConfig()`), and reference them by name and type.

- Add: `resources.add("name", value)`
- Get (internal): `resources.get<T>("name")`
- Types must match exactly (e.g., `std::filesystem::path`, `glm::vec2`, `bool`).

## Session Config JSON

Some apps (e.g. `fingerprint2`) use a *flat session JSON* file as the source-of-truth for startup configuration.

- Location: chosen at startup via file chooser (defaults to `~/Documents`).
- Persistence: the last chosen path is stored at `~/Library/Application Support/<appNamespace>/lastSessionConfig.json`.
- Convention: underscore-prefixed keys are ignored and can be used as “commented out” alternatives.

Helpers:
- `ofxMarkSynth::loadSessionConfigOrExit(...)` (`src/util/SessionConfigUtil.h`) loads `{ path, json }` (hard-exits on error).
- `ofxMarkSynth::loadSessionResourceManagerOrExit(...)` (`src/util/SessionResourceUtil.h`) loads the session config, applies optional runtime settings, and builds a `ResourceManager`.

Optional runtime keys (with defaults):
- `frameRate` (default `30.0`)
- `timeMeasurementsEnabled` (default `false`)
- `logLevel` (default `notice`)
- `logDestination` (`console|gui`, default `console`)

Recording keys:
- `startRecordingOnFirstWake` (default `false`) — start a take on first unpause
- `muxAudioBitrateKbps` (default `192`) — bitrate for muxed audio track

Recording output:
- Takes are saved under `performanceArtefactRootPath/recordings/` as:
  - `take-<timestamp>-composite.mp4`
  - `take-<timestamp>-audio.wav`
  - `take-<timestamp>-raw-video.mp4`
  - `take-<timestamp>-composite-muxed.mp4` (spawned mux step)

Autosnapshot keys:
- `autoSnapshotsEnabled` (default `false`)
- `autoSnapshotsIntervalSec` (default `20.0`)
- `autoSnapshotsJitterSec` (default `7.0`)

Reference example:
- `apps/myApps/fingerprint2/session-config.reference.json`

## Synth Resources

The `Synth` class itself requires several resources for display layout, artefact management, and recording.

### Required Resources

| Resource Name | Type | Description |
|---------------|------|-------------|
| `compositeSize` | glm::vec2 | Fixed composite FBO size (width, height) |
| `startHibernated` | bool | Start in hibernated state (black screen) |
| `compositePanelGapPx` | float | Gap in pixels between the composite panel and side panels |
| `performanceArtefactRootPath` | std::filesystem::path | Base directory for saved artefacts (snapshots, node layouts, videos) |
| `performanceConfigRootPath` | std::filesystem::path | Base directory of performance configs for PerformanceNavigator |

### Optional Resources

| Resource Name | Type | Description |
|---------------|------|-------------|
| `startupPerformanceConfigName` | std::string | Config filename stem from `performanceConfigRootPath/synth` to load on startup (no crossfade). If not found, logs an error and leaves the Synth unloaded. |

### macOS-Only Resources

| Resource Name | Type | Description |
|---------------|------|-------------|
| `recorderCompositeSize` | glm::vec2 | Size (width, height) of the FBO used for composite video recording |
| `ffmpegBinaryPath` | std::filesystem::path | Path to ffmpeg binary used for recording + mux |

### Audio (Required)

Audio is mandatory. `Synth::create()` returns `nullptr` if you don’t provide a valid audio configuration.

Choose one configuration:
- File playback:
  - `sourceAudioPath` (std::filesystem::path)
  - `audioOutDeviceName` (std::string)
  - `audioBufferSize` (int)
  - `audioChannels` (int)
  - `sourceAudioStartPosition` (std::string, optional) — Start playback at this timestamp. Format: `"MM:SS"` (e.g., `"1:30"` for 1 minute 30 seconds).
- Microphone input:
  - `micDeviceName` (std::string)

Recording note:
- For live takes, audio is recorded as **segmented WAV files** controlled by `Synth::toggleRecording()`.

Legacy keys (deprecated):
- `recordAudio` (bool)
- `audioRecordingDir` (string)

Note: `AudioDataSourceMod` reads from the Synth-owned audio analysis client; it does not own the stream or segment recording lifecycle.

Legacy output paths:
- `audioRecordingDir` and `videoRecordingDir` are no longer used by the segmented take recorder.

Example:
```cpp
ofxMarkSynth::ResourceManager resources;

// Required for all platforms
resources.add("compositePanelGapPx", 10.0f);
resources.add("performanceArtefactRootPath", std::filesystem::path(ofToDataPath("artefacts")));
resources.add("performanceConfigRootPath", std::filesystem::path(ofToDataPath("performance-configs")));

// Required on macOS for video recording
resources.add("recorderCompositeSize", glm::vec2(1920, 1080));

resources.add("compositeSize", glm::vec2(1920, 1920));
resources.add("startHibernated", true);  // Start in hibernated state (black screen, press H to start)

auto synth = ofxMarkSynth::Synth::create(
  "MySynth",
  ofxMarkSynth::ModConfig{},
  resources
);
```

---

## Resource Requirements by Mod

### AudioDataSource

`AudioDataSourceMod` does not read audio resources directly. It consumes the Synth-owned audio analysis client created from the Synth audio resources.

---

### VideoFlowSource
Provides optical flow from a video file or camera.

Video ownership:
- Video capture/playback is owned by `Synth` via a persistent `VideoStream`.
- `VideoFlowSourceMod` consumes the Synth-owned stream to perform optical flow analysis.
- The stream continues across config switches (even if a given config does not include a video mod).

Choose one configuration (configured at the **Synth/session** level):
- Video file:
  - `sourceVideoPath` (std::filesystem::path)
  - `sourceVideoMute` (bool)
  - `sourceVideoStartPosition` (std::string, optional) — Start playback at this timestamp. Format: `"MM:SS"`.
- Camera input:
  - `cameraDeviceId` (int)
  - `videoSize` (glm::vec2)  // width, height

Resource note:
- `Synth` publishes `videoStream` (shared) into `ResourceManager`.
- `VideoFlowSourceMod` requires `videoStream`.

Legacy keys (deprecated):
- `saveRecording` (bool)
- `videoRecordingDir` (string)

Example:
```cpp
ofxMarkSynth::ResourceManager resources;
// Video file
resources.add("sourceVideoPath", std::filesystem::path("video/input.mp4"));
resources.add("sourceVideoMute", false);

// OR camera
resources.add("cameraDeviceId", 0);
resources.add("videoSize", glm::vec2(1280, 720));
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

- Source Mods: `AudioDataSource`, `StaticTextSource`, `TimerSource`, `RandomFloatSource`, `RandomHslColor`, `RandomVecSource`
- Process Mods: `Cluster`, `Fluid`, `FluidRadialImpulse`, `MultiplyAdd`, `VectorMagnitude`, `ParticleField`, `ParticleSet`, `Path`, `PixelSnapshot`, `Smear`, `SoftCircle`
- Sink Mods: `Collage`, `DividedArea`, `Introspector`, `SandLine`, `SomPalette`
- Layer Mods: `Fade`

---

## Usage Pattern

```cpp
ofxMarkSynth::ResourceManager resources;

// Synth resources (required)
resources.add("compositePanelGapPx", 10.0f);
resources.add("performanceArtefactRootPath", std::filesystem::path(ofToDataPath("artefacts")));
resources.add("performanceConfigRootPath", std::filesystem::path(ofToDataPath("performance-configs")));
resources.add("recorderCompositeSize", glm::vec2(1920, 1080));  // macOS only
resources.add("compositeSize", glm::vec2(1920, 1920));
resources.add("startHibernated", true);

// Audio resources (required)
resources.add("sourceAudioPath", std::filesystem::path("audio/music.wav"));
resources.add("audioOutDeviceName", std::string("Apple Inc.: MacBook Pro Speakers"));
resources.add("audioBufferSize", 256);
resources.add("audioChannels", 1);

// Add required resources for the Mods you will create
resources.add("fontPath", std::filesystem::path("fonts/Arial.ttf"));

auto synth = ofxMarkSynth::Synth::create(
  "MySynth",
  ofxMarkSynth::ModConfig{},
  resources
);

// If loading from config, make sure resources are set before:
// synth->loadFromConfig(ofToDataPath("configs/my-synth.json"));
```

---

## Notes

- Missing required resources cause Synth/Mod creation to fail; errors are logged.
- Audio is mandatory: missing/invalid audio config makes `Synth::create()` return `nullptr`.
- Use the exact types expected by each Mod.
- Paths should be `std::filesystem::path` for portability.
- `ResourceManager` stores values internally as `std::shared_ptr<T>`; lifetime is managed for you.

## References

- `src/util/ModFactory.hpp` — `ResourceManager` definition
- `src/util/ModFactory.cpp` — Built-in Mod registrations and resource access
- Examples: `example_audio/`, `example_particles_from_video/`, `example_text/`
