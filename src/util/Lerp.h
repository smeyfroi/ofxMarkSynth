//
//  Lerp.h
//  fingerprint1
//
//  Created by Steve Meyfroidt on 31/10/2025.
//

#pragma once

#include "ofColor.h"
#include "ofMath.h"

inline float lerp(const float& a, const float& b, float t) {
  return ofLerp(a, b, t);
}

inline ofFloatColor lerp(const ofFloatColor& a, const ofFloatColor& b, float t) {
  return a.getLerped(b, t);
}

inline glm::vec2 lerp(const glm::vec2& a, const glm::vec2& b, float t) {
  return glm::mix(a, b, t);
}
