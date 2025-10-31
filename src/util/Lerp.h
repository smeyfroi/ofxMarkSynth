//
//  Lerp.h
//  fingerprint1
//
//  Created by Steve Meyfroidt on 31/10/2025.
//

#pragma once

#include "ofColor.h"

inline float lerp(const float& a, const float& b, float t) {
  return ofLerp(a, b, t);
}

inline ofFloatColor lerp(const ofFloatColor& a, const ofFloatColor& b, float t) {
  return a.getLerped(b, t);
}
