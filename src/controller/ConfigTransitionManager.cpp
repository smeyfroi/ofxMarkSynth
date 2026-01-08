//
//  ConfigTransitionManager.cpp
//  ofxMarkSynth
//

#include "controller/ConfigTransitionManager.hpp"
#include "ofGraphics.h"
#include "ofUtils.h"
#include "glm/glm.hpp"

namespace ofxMarkSynth {

void ConfigTransitionManager::captureSnapshot(const ofFbo& sourceFbo) {
    float w = sourceFbo.getWidth();
    float h = sourceFbo.getHeight();
    
    // Allocate snapshot FBO if needed
    if (!snapshotFbo.isAllocated() ||
        snapshotFbo.getWidth() != w ||
        snapshotFbo.getHeight() != h) {
        snapshotFbo.allocate(w, h, GL_RGB16F);
    }
    
    snapshotFbo.begin();
    {
        ofClear(0, 0, 0, 255);
        ofSetColor(255);
        sourceFbo.draw(0, 0);
    }
    snapshotFbo.end();
}

void ConfigTransitionManager::beginTransition() {
    state = State::CROSSFADING;
    startTime = ofGetElapsedTimef();
    alpha = 0.0f;
}

void ConfigTransitionManager::cancelTransition() {
    state = State::NONE;
    alpha = 0.0f;
}

void ConfigTransitionManager::update() {
    if (state != State::CROSSFADING) return;

    float elapsed = ofGetElapsedTimef() - startTime;
    float delaySec = delaySecParameter.get();
    float duration = durationParameter.get();

    // Hold the snapshot at full strength for a moment, so the new config has time
    // to draw something meaningful before we start mixing it in.
    if (elapsed < delaySec) {
        alpha = 0.0f;
        return;
    }

    float t = (elapsed - delaySec) / duration;

    if (t >= 1.0f) {
        alpha = 1.0f;
        state = State::NONE;
    } else {
        alpha = glm::smoothstep(0.0f, 1.0f, t);
    }
}

ConfigTransitionManager::State ConfigTransitionManager::getState() const {
    return state;
}

bool ConfigTransitionManager::isTransitioning() const {
    return state == State::CROSSFADING;
}

float ConfigTransitionManager::getAlpha() const {
    return alpha;
}

const ofFbo& ConfigTransitionManager::getSnapshotFbo() const {
    return snapshotFbo;
}

bool ConfigTransitionManager::hasValidSnapshot() const {
    return snapshotFbo.isAllocated();
}

ofParameter<float>& ConfigTransitionManager::getDurationParameter() {
    return durationParameter;
}

const ofParameter<float>& ConfigTransitionManager::getDurationParameter() const {
    return durationParameter;
}

ofParameter<float>& ConfigTransitionManager::getDelaySecParameter() {
    return delaySecParameter;
}

const ofParameter<float>& ConfigTransitionManager::getDelaySecParameter() const {
    return delaySecParameter;
}

} // namespace ofxMarkSynth
