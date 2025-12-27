//
//  TimeTracker.cpp
//  ofxMarkSynth
//

#include "controller/TimeTracker.hpp"
#include "ofUtils.h"
#include "ofLog.h"

namespace ofxMarkSynth {

void TimeTracker::start() {
    if (everRun) return;
    
    everRun = true;
    worldTimeAtFirstRun = ofGetElapsedTimef();
    synthRunningTime = 0.0f;
    configRunningTime = 0.0f;
    
    ofLogNotice("TimeTracker") << "Started - all time tracking initialized";
}

void TimeTracker::accumulate(float dt) {
    if (!everRun) return;
    
    synthRunningTime += dt;
    configRunningTime += dt;
}

void TimeTracker::resetConfigTime() {
    configRunningTime = 0.0f;
}

bool TimeTracker::hasEverRun() const {
    return everRun;
}

float TimeTracker::getClockTimeSinceFirstRun() const {
    if (!everRun) return 0.0f;
    return ofGetElapsedTimef() - worldTimeAtFirstRun;
}

float TimeTracker::getSynthRunningTime() const {
    return synthRunningTime;
}

float TimeTracker::getConfigRunningTime() const {
    return configRunningTime;
}

int TimeTracker::getConfigRunningMinutes() const {
    return static_cast<int>(configRunningTime) / 60;
}

int TimeTracker::getConfigRunningSeconds() const {
    return static_cast<int>(configRunningTime) % 60;
}

} // namespace ofxMarkSynth
