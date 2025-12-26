//
//  HibernationController.cpp
//  ofxMarkSynth
//

#include "HibernationController.hpp"
#include "ofUtils.h"

namespace ofxMarkSynth {

HibernationController::HibernationController(const std::string& synthName_, bool startHibernated)
    : synthName(synthName_)
    , state(startHibernated ? State::HIBERNATED : State::ACTIVE)
    , alpha(startHibernated ? 0.0f : 1.0f)
    , fadeStartTime(0.0f)
{
}

bool HibernationController::start() {
    if (state != State::ACTIVE) return false;
    
    ofLogNotice("HibernationController") << "Starting hibernation, fade duration: "
                                         << fadeDurationParameter.get() << "s";
    state = State::FADING_OUT;
    fadeStartTime = ofGetElapsedTimef();
    return true;  // Caller should pause
}

bool HibernationController::cancel() {
    if (state == State::ACTIVE) return false;
    
    if (state == State::FADING_OUT) {
        ofLogNotice("HibernationController") << "Cancelling hibernation";
    }
    
    state = State::ACTIVE;
    alpha = 1.0f;
    return true;  // Caller should unpause
}

void HibernationController::update() {
    if (state != State::FADING_OUT) return;
    
    float elapsed = ofGetElapsedTimef() - fadeStartTime;
    float duration = fadeDurationParameter.get();
    
    if (elapsed >= duration) {
        alpha = 0.0f;
        state = State::HIBERNATED;
        
        ofLogNotice("HibernationController") << "Hibernation complete after " << elapsed << "s";
        
        CompleteEvent event;
        event.fadeDuration = elapsed;
        event.synthName = synthName;
        ofNotifyEvent(completeEvent, event, this);
    } else {
        float t = elapsed / duration;
        alpha = 1.0f - t;
    }
}

HibernationController::State HibernationController::getState() const {
    return state;
}

bool HibernationController::isHibernating() const {
    return state != State::ACTIVE;
}

float HibernationController::getAlpha() const {
    return alpha;
}

std::string HibernationController::getStateString() const {
    switch (state) {
        case State::ACTIVE:     return "Active";
        case State::FADING_OUT: return "Hibernating...";
        case State::HIBERNATED: return "Hibernated";
    }
    return "Unknown";
}

ofParameter<float>& HibernationController::getFadeDurationParameter() {
    return fadeDurationParameter;
}

const ofParameter<float>& HibernationController::getFadeDurationParameter() const {
    return fadeDurationParameter;
}

} // namespace ofxMarkSynth
