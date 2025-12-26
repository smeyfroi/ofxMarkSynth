//
//  ConfigTransitionManager.cpp
//  ofxMarkSynth
//

#include "ConfigTransitionManager.hpp"
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
    float duration = durationParameter.get();
    
    if (elapsed >= duration) {
        alpha = 1.0f;
        state = State::NONE;
    } else {
        float t = elapsed / duration;
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

} // namespace ofxMarkSynth
