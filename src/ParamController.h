//
//  ParamController.h
//  fingerprint1
//
//  Created by Steve Meyfroidt on 27/10/2025.
//

#pragma once

#include "ofMath.h"
#include "ofColor.h"



namespace ofxMarkSynth {



inline float lerp(const float& a, const float& b, float t) { return ofLerp(a, b, t); }
inline float clamp(const float& v, const float& lo, const float& hi) { return ofClamp(v, lo, hi); }

inline ofFloatColor lerp(const ofFloatColor& a, const ofFloatColor& b, float t) { return a.getLerped(b, t); }
inline ofFloatColor clamp(const ofFloatColor& v, const ofFloatColor& lo, const ofFloatColor& hi) {
    return ofFloatColor(ofClamp(v.r, lo.r, hi.r),
                        ofClamp(v.g, lo.g, hi.g),
                        ofClamp(v.b, lo.b, hi.b),
                        ofClamp(v.a, lo.a, hi.a));
}

template<typename T>
class ParamController {
public:
  ParamController(ofParameter<T>& manualValueParameter_) :
  manualValueParameter(manualValueParameter_)
  {
    update(manualValueParameter.get(), 0.0f);
  }

  void update(T newValue, float agency) { // agency 0.0 = full manual, 1.0 = full parameter
    float timestamp = ofGetElapsedTimef();
    float dt = timestamp - lastTimeUpdated;
    
    // Blend manual and parameter values
    T target = lerp(manualValueParameter.get(), newValue, agency);
    
    // Avoid large jumps
    float maxStep = rateLimitPerSec * dt;
    target = clamp(target, value - maxStep, value + maxStep);
    
    // One-pole smooth
    float alpha = 1.0f - expf(-dt / std::max(1e-4f, smoothingTauSecs));
    value = lerp(value, target, alpha);
    
    lastTimeUpdated = timestamp;
  }
  
  void update(float agency) {
    update(value, agency);
  }

  ofParameter<T>& manualValueParameter;
  T value;
  float lastTimeUpdated;
  float smoothingTauSecs = 0.15f;
  float rateLimitPerSec = 1.0f; // max change per second
};



} // ofxMarkSynth
