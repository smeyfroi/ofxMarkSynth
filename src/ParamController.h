#pragma once

#include "ofMain.h"

namespace ofxMarkSynth {

inline float lerp(const float& a, const float& b, float t) {
  return ofLerp(a, b, t);
}

inline ofFloatColor lerp(const ofFloatColor& a, const ofFloatColor& b, float t) {
  return a.getLerped(b, t);
}

template<typename T>
class ParamController {
public:
  ParamController(ofParameter<T>& manualValueParameter_)
  : manualValueParameter(manualValueParameter_),
  value(manualValueParameter_.get()),
  intentValue(manualValueParameter_.get()),
  autoValue(manualValueParameter_.get()),
  agency(0.0f),
  intentStrength(0.0f)
  {
  }
  
  void updateIntentAuto(T newIntentValue, T newAutoValue,
                        float newIntentStrength, float newAgency) {
    intentValue = newIntentValue;
    autoValue = newAutoValue;
    intentStrength = newIntentStrength;
    agency = newAgency;
    update();
  }
  
  void updateIntent(T newIntentValue, float newIntentStrength) {
    intentValue = newIntentValue;
    intentStrength = newIntentStrength;
    update();
  }
  
  void updateAuto(T newAutoValue, float newAgency) {
    autoValue = newAutoValue;
    agency = newAgency;
    update();
  }
  
  void update() {
    T manualValue = manualValueParameter.get();
    T intentBlended = lerp(manualValue, intentValue, intentStrength);
    T targetValue = lerp(intentBlended, autoValue, agency);
    
    smoothTowards(targetValue);
  }
  
  ofParameter<T>& manualValueParameter;
  T value;
  T intentValue;
  T autoValue;
  float agency;
  float intentStrength;
  float smoothTime { 0.3f };
  
private:
  void smoothTowards(T targetValue) {
    float dt = ofGetLastFrameTime();
    float alpha = 1.0f - expf(-dt / smoothTime);
    value = lerp(value, targetValue, alpha);
  }
};

} // namespace ofxMarkSynth
