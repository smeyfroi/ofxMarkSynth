#pragma once

#include "Intent.hpp"

namespace ofxMarkSynth {

inline float linearMap(float intentValue, float minOut, float maxOut) {
  return ofLerp(minOut, maxOut, intentValue);
}

inline float exponentialMap(float intentValue, float minOut, float maxOut, float exponent = 2.0) {
  float curved = powf(ofClamp(intentValue, 0.0, 1.0), exponent);
  return ofLerp(minOut, maxOut, curved);
}

inline float inverseMap(float intentValue, float minOut, float maxOut) {
  return ofLerp(maxOut, minOut, intentValue);
}

inline ofFloatColor energyToColor(const Intent& intent) {
  float e = intent.getEnergy();
  return ofFloatColor {
    ofLerp(0.3, 1.0, e),
    ofLerp(0.3, 0.8, e),
    ofLerp(0.5, 0.3, e),
    ofLerp(0.5, 1.0, e)
  };
}

inline float structureToBrightness(const Intent& intent) {
  float s = intent.getStructure();
  return ofLerp(0.0, 0.2, s);
}

inline ofFloatColor densityToAlpha(const Intent& intent, const ofFloatColor& baseColor) {
  float d = intent.getDensity();
  ofFloatColor result = baseColor;
  result.a = ofLerp(0.3, 1.0, d);
  return result;
}

} // namespace ofxMarkSynth
