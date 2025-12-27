//
//  RenderingConstants.h
//  ofxMarkSynth
//
//  Named constants for rendering-related values.
//

#pragma once

namespace ofxMarkSynth {

// Video recording defaults (macOS VideoToolbox)
constexpr float DEFAULT_VIDEO_FPS = 30.0f;
constexpr int DEFAULT_VIDEO_BITRATE = 8000;
constexpr const char* DEFAULT_VIDEO_CODEC = "h264_videotoolbox";

// Side panel update timeouts (seconds)
constexpr float LEFT_PANEL_TIMEOUT_SECS = 7.0f;
constexpr float RIGHT_PANEL_TIMEOUT_SECS = 11.0f;

// Async image saver PBO timing
constexpr int PBO_FRAMES_TO_WAIT = 2;
constexpr int PBO_MAX_FRAMES_BEFORE_ABANDON = 10;

// Side panel random position bounds (fraction of composite)
constexpr float PANEL_ORIGIN_MIN_FRAC = 0.25f;  // 1/4 from edge
constexpr float PANEL_ORIGIN_MAX_FRAC = 0.75f;  // 3/4 from edge

} // namespace ofxMarkSynth
