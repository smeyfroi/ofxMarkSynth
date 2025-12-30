//
//  HibernationController.cpp
//  ofxMarkSynth
//

#include "controller/HibernationController.hpp"
#include "ofUtils.h"

namespace ofxMarkSynth {

HibernationController::HibernationController(const std::string& synthName_, bool startHibernated)
    : synthName(synthName_)
    , state(startHibernated ? State::HIBERNATED : State::ACTIVE)
    , alpha(startHibernated ? 0.0f : 1.0f)
    , fadeStartTime(0.0f)
    , fadeStartAlpha(startHibernated ? 0.0f : 1.0f)
{
}

bool HibernationController::hibernate() {
    if (state == State::HIBERNATED) return false;
    
    if (state == State::FADING_IN) {
        // Reverse direction: start fading out from current alpha
        ofLogNotice("HibernationController") << "Reversing fade-in to fade-out at alpha " << alpha;
        state = State::FADING_OUT;
        fadeStartTime = ofGetElapsedTimef();
        fadeStartAlpha = alpha;
        return true;
    }
    
    if (state == State::ACTIVE) {
        ofLogNotice("HibernationController") << "Starting hibernation, fade duration: "
                                             << fadeOutDurationParameter.get() << "s";
        state = State::FADING_OUT;
        fadeStartTime = ofGetElapsedTimef();
        fadeStartAlpha = 1.0f;
        return true;
    }
    
    // Already FADING_OUT, no change needed
    return false;
}

bool HibernationController::wake() {
    if (state == State::ACTIVE) return false;
    
    if (state == State::FADING_OUT) {
        // Reverse direction: start fading in from current alpha
        ofLogNotice("HibernationController") << "Reversing fade-out to fade-in at alpha " << alpha;
        state = State::FADING_IN;
        fadeStartTime = ofGetElapsedTimef();
        fadeStartAlpha = alpha;
        return true;
    }
    
    if (state == State::HIBERNATED) {
        ofLogNotice("HibernationController") << "Waking from hibernation, fade duration: "
                                             << fadeInDurationParameter.get() << "s";
        state = State::FADING_IN;
        fadeStartTime = ofGetElapsedTimef();
        fadeStartAlpha = 0.0f;
        return true;
    }
    
    // Already FADING_IN, no change needed
    return false;
}

void HibernationController::update() {
    if (state == State::FADING_OUT) {
        float elapsed = ofGetElapsedTimef() - fadeStartTime;
        // Calculate duration proportionally based on how much alpha we need to fade
        float fullDuration = fadeOutDurationParameter.get();
        float proportionalDuration = fadeStartAlpha * fullDuration;
        
        if (elapsed >= proportionalDuration) {
            alpha = 0.0f;
            state = State::HIBERNATED;
            
            ofLogNotice("HibernationController") << "Hibernation complete after " << elapsed << "s";
            
            CompleteEvent event;
            event.fadeDuration = elapsed;
            event.synthName = synthName;
            ofNotifyEvent(completeEvent, event, this);
        } else {
            float t = elapsed / proportionalDuration;
            alpha = fadeStartAlpha * (1.0f - t);
        }
    } else if (state == State::FADING_IN) {
        float elapsed = ofGetElapsedTimef() - fadeStartTime;
        // Calculate duration proportionally based on how much alpha we need to fade
        float fullDuration = fadeInDurationParameter.get();
        float alphaToFade = 1.0f - fadeStartAlpha;
        float proportionalDuration = alphaToFade * fullDuration;
        
        if (elapsed >= proportionalDuration) {
            alpha = 1.0f;
            state = State::ACTIVE;
            
            ofLogNotice("HibernationController") << "Wake complete after " << elapsed << "s";
        } else {
            float t = elapsed / proportionalDuration;
            alpha = fadeStartAlpha + alphaToFade * t;
        }
    }
}

HibernationController::State HibernationController::getState() const {
    return state;
}

bool HibernationController::isHibernating() const {
    return state != State::ACTIVE;
}

bool HibernationController::isFullyHibernated() const {
    return state == State::HIBERNATED;
}

bool HibernationController::isFading() const {
    return state == State::FADING_OUT || state == State::FADING_IN;
}

float HibernationController::getAlpha() const {
    return alpha;
}

std::string HibernationController::getStateString() const {
    switch (state) {
        case State::ACTIVE:     return "Active";
        case State::FADING_OUT: return "Hibernating...";
        case State::HIBERNATED: return "Hibernated";
        case State::FADING_IN:  return "Waking...";
    }
    return "Unknown";
}

ofParameter<float>& HibernationController::getFadeOutDurationParameter() {
    return fadeOutDurationParameter;
}

const ofParameter<float>& HibernationController::getFadeOutDurationParameter() const {
    return fadeOutDurationParameter;
}

ofParameter<float>& HibernationController::getFadeInDurationParameter() {
    return fadeInDurationParameter;
}

const ofParameter<float>& HibernationController::getFadeInDurationParameter() const {
    return fadeInDurationParameter;
}

} // namespace ofxMarkSynth
