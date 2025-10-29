#pragma once

#include "ofMath.h"
#include "ofColor.h"
#include <type_traits>
#include <cmath>

namespace ofxMarkSynth {



inline float lerp(const float& a, const float& b, float t) { return ofLerp(a, b, t); }
inline ofFloatColor lerp(const ofFloatColor& a, const ofFloatColor& b, float t) { return a.getLerped(b, t); }



template<typename T>
class ParamController {
public:
  ParamController(ofParameter<T>& manualValueParameter_)
  : manualValueParameter(manualValueParameter_),
    value(manualValueParameter_.get()),
    autoValue(manualValueParameter_.get())
  {
  }

  void update(T newValue, float autoManualMix) {
    autoValue = newValue;
    tick(autoManualMix);
  }

  void update(float autoManualMix) {
    tick(autoManualMix);
  }

private:
  void tick(float autoManualMix) {
    T targetMixed = lerp(manualValueParameter.get(), autoValue, autoManualMix);

    float dt = ofGetLastFrameTime();
    float alpha = 1.0f - expf(-dt / smoothTime);

    value = lerp(value, targetMixed, alpha);
  }

public:
  ofParameter<T>& manualValueParameter;
  T value;
  T autoValue;
  float smoothTime { 0.2f };
};



} // namespace ofxMarkSynth
