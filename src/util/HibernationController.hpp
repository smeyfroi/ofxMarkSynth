//
//  HibernationController.hpp
//  ofxMarkSynth
//

#pragma once

#include "ofParameter.h"
#include "ofEvents.h"
#include <string>

namespace ofxMarkSynth {

/// Manages the fade-to-black hibernation state machine.
/// States: ACTIVE -> FADING_OUT -> HIBERNATED
class HibernationController {
public:
    enum class State {
        ACTIVE,
        FADING_OUT,
        HIBERNATED
    };

    class CompleteEvent : public ofEventArgs {
    public:
        float fadeDuration;
        std::string synthName;
    };
    ofEvent<CompleteEvent> completeEvent;

    HibernationController(const std::string& synthName, bool startHibernated);

    /// Begin fade to black. Returns true if caller should pause.
    bool start();

    /// Snap back to active. Returns true if caller should unpause.
    bool cancel();

    /// Advance fade animation (call each frame).
    void update();

    State getState() const;
    bool isHibernating() const;
    float getAlpha() const;
    std::string getStateString() const;

    ofParameter<float>& getFadeDurationParameter();
    const ofParameter<float>& getFadeDurationParameter() const;

private:
    std::string synthName;
    State state;
    float alpha;
    float fadeStartTime;
    ofParameter<float> fadeDurationParameter { "Hibernate Duration", 2.0f, 0.5f, 10.0f };
};

} // namespace ofxMarkSynth
