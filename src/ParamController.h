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
inline float clampTargetToMaxStep(const float& t, const float& v, const float& maxStep) { return ofClamp(t, v - maxStep, v + maxStep); }

inline ofFloatColor lerp(const ofFloatColor& a, const ofFloatColor& b, float t) { return a.getLerped(b, t); }
inline ofFloatColor clampTargetToMaxStep(const ofFloatColor& t, const ofFloatColor& v, const float& maxStep) {
    return ofFloatColor(ofClamp(t.r, v.r - maxStep, v.r + maxStep),
                        ofClamp(t.g, v.g - maxStep, v.g + maxStep),
                        ofClamp(t.b, v.b - maxStep, v.b + maxStep),
                        ofClamp(t.a, v.a - maxStep, v.a + maxStep));
}

template<typename T>
class ParamController {
public:
  ParamController(ofParameter<T>& manualValueParameter_) :
  manualValueParameter(manualValueParameter_),
  lastTimeUpdated(0.0f)
  {
    update(manualValueParameter.get(), 0.0f);
  }

  void update(T newValue, float agency) { // agency 0.0 = full manual, 1.0 = full parameter
    float timestamp = ofGetElapsedTimef();
    float dt = timestamp - lastTimeUpdated;
    
    // Blend manual and parameter values
    T target = lerp(manualValueParameter.get(), newValue, agency);
    
    // Rate limit (faster for colors)
    float maxStep = (std::is_same_v<T, ofFloatColor> ? rateLimitPerSecColor : rateLimitPerSec) * dt;
    target = clampTargetToMaxStep(target, value, maxStep);

    // One-pole smoothing (faster for colors)
    float tau = std::max(1e-4f, std::is_same_v<T, ofFloatColor> ? smoothingTauSecsColor : smoothingTauSecs);
    float alpha = 1.0f - expf(-dt / tau);
    value = lerp(value, target, alpha);

    lastTimeUpdated = timestamp;
  }
  
  void update(float agency) {
    update(value, agency);
  }

  ofParameter<T>& manualValueParameter;
  T value;
  float lastTimeUpdated;
  float smoothingTauSecsColor = 0.00001f; // smaller => snappier
  float rateLimitPerSecColor = 1000.0f; // higher => larger allowed step
  float smoothingTauSecs = 0.01f; // smaller => snappier
  float rateLimitPerSec = 10.0f; // higher => larger allowed step
};



} // ofxMarkSynth
