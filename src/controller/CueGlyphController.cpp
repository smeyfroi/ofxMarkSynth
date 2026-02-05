//
//  CueGlyphController.cpp
//  ofxMarkSynth
//

#include "controller/CueGlyphController.hpp"

#include <algorithm>

#include "ofGraphics.h"

namespace ofxMarkSynth {

namespace {

constexpr float ICON_SIZE_PX = 26.0f;
constexpr float MARGIN_PX = 14.0f;
constexpr float STROKE_PX = 2.0f;
constexpr float TIMING_BAR_HEIGHT_PX = 3.0f;
constexpr float TIMING_BAR_GAP_PX = 6.0f;

void drawAudioBars(float x, float y, float size, float peakHeightFactor = 0.85f) {
  const float barW = size * 0.12f;
  const float gap = barW * 0.75f;

  // Base bar proportions; we scale them down for the merged A+V icon
  // to avoid the tallest bar touching the video-frame border.
  constexpr float BASE_PEAK = 0.85f;
  float scale = peakHeightFactor / BASE_PEAK;
  const float heights[] = { 0.45f * scale, 0.85f * scale, 0.60f * scale };

  for (int i = 0; i < 3; ++i) {
    float h = size * heights[i];
    float bx = x + i * (barW + gap);
    float by = y + size - h;
    ofDrawRectangle(bx, by, barW, h);
  }
}

void drawVideoFrame(float x, float y, float size, bool includeLens) {
  const float frameH = size * 0.70f;
  const float frameY = y + (size - frameH) * 0.5f;

  ofNoFill();
  ofSetLineWidth(STROKE_PX);
  ofDrawRectangle(x, frameY, size, frameH);

  if (includeLens) {
    ofFill();
    float r = size * 0.06f;
    ofDrawCircle(x + size * 0.82f, frameY + frameH * 0.28f, r);
    ofNoFill();
  }
}

} // namespace

void CueGlyphController::draw(const DrawParams& params, float windowWidth, float windowHeight) const {
  if (!params.audioEnabled && !params.videoEnabled) return;

  const float alpha = std::clamp(params.alpha, 0.0f, 1.0f);
  if (alpha <= 0.0f) return;

  const float x = MARGIN_PX;
  const float y = windowHeight - MARGIN_PX - ICON_SIZE_PX;

  ofPushStyle();
  ofEnableAlphaBlending();

  // Timing cue (drawn above icon)
  if (params.flashExpired) {
    ofFill();
    ofSetColor(255, 80, 80, static_cast<unsigned char>(255.0f * alpha));
    float barY = y - TIMING_BAR_GAP_PX - TIMING_BAR_HEIGHT_PX;
    ofDrawRectangle(x, barY, ICON_SIZE_PX, TIMING_BAR_HEIGHT_PX);
  } else if (params.imminentConfigChangeProgress > 0.0f) {
    float p = std::clamp(params.imminentConfigChangeProgress, 0.0f, 1.0f);
    ofFill();
    ofSetColor(255, 255, 255, static_cast<unsigned char>(200.0f * alpha));
    float barY = y - TIMING_BAR_GAP_PX - TIMING_BAR_HEIGHT_PX;
    ofDrawRectangle(x, barY, ICON_SIZE_PX * p, TIMING_BAR_HEIGHT_PX);
  }

  // Main icon
  ofFill();
  ofSetColor(255, 255, 255, static_cast<unsigned char>(255.0f * alpha));

  if (params.audioEnabled && params.videoEnabled) {
    drawVideoFrame(x, y, ICON_SIZE_PX, true);
    ofFill();
    float inset = ICON_SIZE_PX * 0.12f;
    drawAudioBars(x + inset, y, ICON_SIZE_PX - inset * 2.0f, 0.72f);
  } else if (params.videoEnabled) {
    drawVideoFrame(x, y, ICON_SIZE_PX, true);
  } else if (params.audioEnabled) {
    drawAudioBars(x, y, ICON_SIZE_PX);
  }

  ofPopStyle();
}

} // namespace ofxMarkSynth
