//
//  SynthConstants.h
//  ofxMarkSynth
//
//  Named constants for core Synth-related values.
//

#pragma once

namespace ofxMarkSynth {

// Mod/Layer ID allocation
// Mods get positive IDs starting at 1000, incrementing by 1000
// DrawingLayers get negative IDs starting at -1000
constexpr int MOD_ID_START = 1000;
constexpr int MOD_ID_INCREMENT = 1000;
constexpr int DRAWING_LAYER_ID_START = -1000;

// Controller thresholds
constexpr float FLOAT_EPSILON = 0.0001f;
constexpr float CONTROLLER_CHANGE_THRESHOLD = 0.005f;
constexpr float NORMALIZATION_EPSILON = 1e-6f;

// Intent system
constexpr int MAX_INTENT_SLOTS = 7;

// Autosave (full-res HDR EXR) - not exposed in GUI.
// NOTE: Autosaves are also gated by runtime conditions (paused/hibernation, no overlap,
// manual save priority). See Synth::update().
// Set false during tuning/development.
constexpr bool AUTO_SNAPSHOTS_ENABLED = false;
constexpr float AUTO_SNAPSHOTS_INTERVAL_SEC = 20.0f;
constexpr float AUTO_SNAPSHOTS_JITTER_SEC = 7.0f;

} // namespace ofxMarkSynth
