//
//  TimeTracker.hpp
//  ofxMarkSynth
//

#pragma once

namespace ofxMarkSynth {

/// Tracks three time values for the synth:
/// 1. Clock time since first run (wall clock, never pauses)
/// 2. Synth running time (accumulates when not paused)
/// 3. Config running time (accumulates when not paused, resets on config switch)
class TimeTracker {
public:
    TimeTracker() = default;

    /// Called once when synth first starts running
    void start();

    /// Called each frame with capped delta time when synth is running
    void accumulate(float dt);

    /// Called on config switch to reset config-specific time
    void resetConfigTime();

    bool hasEverRun() const;
    float getClockTimeSinceFirstRun() const;
    float getSynthRunningTime() const;
    float getConfigRunningTime() const;
    int getConfigRunningMinutes() const;
    int getConfigRunningSeconds() const;

private:
    bool everRun { false };
    float worldTimeAtFirstRun { 0.0f };
    float synthRunningTime { 0.0f };
    float configRunningTime { 0.0f };
};

} // namespace ofxMarkSynth
