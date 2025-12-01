//
//  Lerp.h
//  fingerprint1
//
//  Created by Steve Meyfroidt on 31/10/2025.
//

#pragma once

#include "ofColor.h"
#include "ofMath.h"
#include "Oklab.h"
#include <cmath>

inline float lerp(const float& a, const float& b, float t) {
  return ofLerp(a, b, t);
}

inline glm::vec2 lerp(const glm::vec2& a, const glm::vec2& b, float t) {
  return glm::mix(a, b, t);
}

// =============================================================================
// Perceptual color interpolation using Oklab
// =============================================================================

// Perceptually uniform color interpolation via Oklab color space
inline ofFloatColor lerpPerceptual(const ofFloatColor& a, const ofFloatColor& b, float t) {
  ofxMarkSynth::Oklab labA = ofxMarkSynth::rgbToOklab(a);
  ofxMarkSynth::Oklab labB = ofxMarkSynth::rgbToOklab(b);
  ofxMarkSynth::Oklab result {
    ofLerp(labA.L, labB.L, t),
    ofLerp(labA.a, labB.a, t),
    ofLerp(labA.b, labB.b, t)
  };
  float alpha = ofLerp(a.a, b.a, t);
  return ofxMarkSynth::oklabToRgb(result, alpha);
}

// Default color lerp now uses perceptual interpolation
inline ofFloatColor lerp(const ofFloatColor& a, const ofFloatColor& b, float t) {
  return lerpPerceptual(a, b, t);
}

// =============================================================================
// Weighted blending for colors (handles all RGBA channels properly)
// =============================================================================

// Weighted blend of three colors with proper RGBA handling
// Note: ofFloatColor's arithmetic operators ignore alpha, so we handle it explicitly
inline ofFloatColor weightedBlend(const ofFloatColor& a, float wA,
                                   const ofFloatColor& b, float wB,
                                   const ofFloatColor& c, float wC) {
  return ofFloatColor {
    a.r * wA + b.r * wB + c.r * wC,
    a.g * wA + b.g * wB + c.g * wC,
    a.b * wA + b.b * wB + c.b * wC,
    a.a * wA + b.a * wB + c.a * wC
  };
}

// =============================================================================
// Angular interpolation for cyclic values (e.g., hue in [0, 1])
// =============================================================================

// Linear interpolation taking the shortest path around a circle (for values in [0, 1])
inline float lerpAngular(float a, float b, float t) {
  // Find the shortest signed distance around the circle
  float diff = b - a;
  
  // Wrap diff to [-0.5, 0.5] (shortest path)
  if (diff > 0.5f) diff -= 1.0f;
  else if (diff < -0.5f) diff += 1.0f;
  
  float result = a + diff * t;
  
  // Wrap result back to [0, 1)
  result = std::fmod(result, 1.0f);
  if (result < 0.0f) result += 1.0f;
  
  return result;
}
