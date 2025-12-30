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
/// States: ACTIVE <-> FADING_OUT <-> HIBERNATED <-> FADING_IN <-> ACTIVE
/// Fades can be reversed mid-transition by calling the opposite action.
class HibernationController {
public:
    enum class State {
        ACTIVE,
        FADING_OUT,
        HIBERNATED,
        FADING_IN
    };

    class CompleteEvent : public ofEventArgs {
    public:
        float fadeDuration;
        std::string synthName;
    };
    ofEvent<CompleteEvent> completeEvent;

    HibernationController(const std::string& synthName, bool startHibernated);

    /// Begin fade to black (or reverse fade-in). Returns true if state changed.
    bool hibernate();

    /// Begin fade from black (or reverse fade-out). Returns true if state changed.
    bool wake();

    /// Advance fade animation (call each frame).
    void update();

    State getState() const;
    
    /// Returns true if not fully ACTIVE (i.e., fading or hibernated)
    bool isHibernating() const;
    
    /// Returns true if fully hibernated (screen completely black)
    bool isFullyHibernated() const;
    
    /// Returns true if in a fade transition (FADING_OUT or FADING_IN)
    bool isFading() const;
    
    float getAlpha() const;
    std::string getStateString() const;

    ofParameter<float>& getFadeOutDurationParameter();
    const ofParameter<float>& getFadeOutDurationParameter() const;
    ofParameter<float>& getFadeInDurationParameter();
    const ofParameter<float>& getFadeInDurationParameter() const;

private:
    std::string synthName;
    State state;
    float alpha;
    float fadeStartTime;
    float fadeStartAlpha;  // Alpha when fade started (for reversing mid-fade)
    ofParameter<float> fadeOutDurationParameter { "Hibernate Fade Out", 2.0f, 0.5f, 10.0f };
    ofParameter<float> fadeInDurationParameter { "Hibernate Fade In", 1.0f, 0.1f, 5.0f };
};

} // namespace ofxMarkSynth
