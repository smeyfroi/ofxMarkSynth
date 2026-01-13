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
    snapshotWeight = 1.0f;
    liveWeight = 0.0f;
    alpha = 0.0f;
}

void ConfigTransitionManager::cancelTransition() {
    state = State::NONE;
    snapshotWeight = 1.0f;
    liveWeight = 0.0f;
    alpha = 0.0f;
}

void ConfigTransitionManager::update() {
    if (state != State::CROSSFADING) return;

    float elapsed = ofGetElapsedTimef() - startTime;
    float delaySec = delaySecParameter.get();
    float duration = durationParameter.get();

    // During the delay window we show only the snapshot (old config), giving the
    // new config a moment to start rendering before it becomes visible.
    if (elapsed < delaySec) {
        snapshotWeight = 1.0f;
        liveWeight = 0.0f;
        alpha = 0.0f;
        return;
    }

    float t = ofClamp((elapsed - delaySec) / duration, 0.0f, 1.0f);

    if (t >= 1.0f) {
        snapshotWeight = 0.0f;
        liveWeight = 1.0f;
        alpha = 1.0f;
        state = State::NONE;
        return;
    }

    // Weighted crossfade: bring the live config in quickly, while keeping the
    // snapshot visually dominant for a little longer.
    constexpr float SNAPSHOT_START_WEIGHT = 2.0f;
    constexpr float LIVE_MAX_WEIGHT = 1.0f;
    constexpr float LIVE_RAMP_FRAC = 0.2f;        // live reaches full strength quickly
    constexpr float SNAPSHOT_FADE_START = 0.4f;   // snapshot holds longer, then fades

    float liveRampT = ofClamp(t / LIVE_RAMP_FRAC, 0.0f, 1.0f);
    liveWeight = LIVE_MAX_WEIGHT * glm::smoothstep(0.0f, 1.0f, liveRampT);

    float snapshotFadeT = ofClamp((t - SNAPSHOT_FADE_START) / (1.0f - SNAPSHOT_FADE_START), 0.0f, 1.0f);
    snapshotWeight = SNAPSHOT_START_WEIGHT * (1.0f - glm::smoothstep(0.0f, 1.0f, snapshotFadeT));

    float weightSum = snapshotWeight + liveWeight;
    alpha = (weightSum > 0.0f) ? (liveWeight / weightSum) : 1.0f;
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

float ConfigTransitionManager::getSnapshotWeight() const {
    return snapshotWeight;
}

float ConfigTransitionManager::getLiveWeight() const {
    return liveWeight;
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
