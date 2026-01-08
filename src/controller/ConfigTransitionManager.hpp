//
//  ConfigTransitionManager.hpp
//  ofxMarkSynth
//

#pragma once

#include "ofFbo.h"
#include "ofParameter.h"

namespace ofxMarkSynth {

/// Manages crossfade transitions between config switches.
/// Captures a snapshot of the old config's final frame and crossfades to the new config.
class ConfigTransitionManager {
public:
    enum class State {
        NONE,
        CROSSFADING
    };

    ConfigTransitionManager() = default;

    /// Capture the current frame from source FBO before config switch
    void captureSnapshot(const ofFbo& sourceFbo);

    /// Begin the crossfade transition
    void beginTransition();

    /// Cancel any in-progress transition
    void cancelTransition();

    /// Update transition alpha (call each frame)
    void update();

    // State accessors
    State getState() const;
    bool isTransitioning() const;
    float getAlpha() const;  // 0.0 = all snapshot, 1.0 = all live

    // FBO access for rendering
    const ofFbo& getSnapshotFbo() const;
    bool hasValidSnapshot() const;

    // Parameters for GUI/serialization
    ofParameter<float>& getDurationParameter();
    const ofParameter<float>& getDurationParameter() const;
    ofParameter<float>& getDelaySecParameter();
    const ofParameter<float>& getDelaySecParameter() const;

private:
    State state { State::NONE };
    ofFbo snapshotFbo;
    float startTime { 0.0f };
    float alpha { 0.0f };
    ofParameter<float> delaySecParameter { "Crossfade Delay Sec", 0.5f, 0.0f, 9999.0f };
    ofParameter<float> durationParameter { "Crossfade Duration", 2.5f, 0.5f, 10.0f };
};

} // namespace ofxMarkSynth
