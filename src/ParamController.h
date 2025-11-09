#pragma once

#include "ofMain.h"
#include "Lerp.h"



namespace ofxMarkSynth {



// This only exists so that we can introspect the weights for Gui without needing to templatize everything
class BaseParamController {
public:
  virtual ~BaseParamController() = default;
  float wAuto, wManual, wIntent;
};



template<typename T>
class ParamController : public BaseParamController {
public:
  ParamController(ofParameter<T>& manualValueParameter_)
  : manualValueParameter(manualValueParameter_),
  value(manualValueParameter_.get()),
  intentValue(manualValueParameter_.get()),
  autoValue(manualValueParameter_.get()),
  autoSmoothed(manualValueParameter_.get()),
  intentSmoothed(manualValueParameter_.get()),
  manualSmoothed(manualValueParameter_.get()),
  agency(0.0f),
  intentStrength(0.0f),
  lastManualUpdateTime(0.0f)
  {
    paramListener = manualValueParameter.newListener([this](T&) {
      lastManualUpdateTime = ofGetElapsedTimef();
    });
  }
  
  T getManualMin() const {
    return manualValueParameter.getMin();
  }
  
  T getManualMax() const {
    return manualValueParameter.getMax();
  }
  
  float getTimeSinceLastManualUpdate() const {
    return ofGetElapsedTimef() - lastManualUpdateTime;
  }
  
  bool isManualControlActive(float thresholdTime = 0.2) const {
    return getTimeSinceLastManualUpdate() < thresholdTime;
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

  // TODO: push to util/Lerp.h
  inline float oneMinusExp(float dt, float tau) {
    if (tau <= 0.0f) return 1.0f;
    return 1.0f - std::exp(-dt / tau);
  }

  // TODO: push to util/Lerp.h
  inline T smoothTo(T current, T target, float dt, float tau) {
    float alpha = oneMinusExp(dt, tau);
    return lerp(value, target, alpha);
  }

  // TODO: push to util/Lerp.h
  inline float smoothToFloat(float current, float target, float dt, float tau) {
    float alpha = oneMinusExp(dt, tau);
    return ofLerp(current, target, alpha);
  }
  
  void update() {
    float dt = ofGetLastFrameTime();

    manualBias = isManualControlActive() ? 1.0f : smoothToFloat(manualBias, baseManualBias, dt, manualBiasDecaySec);
    manualSmoothed = smoothTo(manualSmoothed, manualValueParameter.get(), dt, manualSmoothSec);
    autoSmoothed   = smoothTo(autoSmoothed, autoValue, dt, autoSmoothSec);
    intentSmoothed = smoothTo(intentSmoothed, intentValue, dt, intentSmoothSec);

    // --- Weighting: outer (auto vs human), inner (manual vs intent) ---
    float humanShare = 1.0f - agency; // long-term machine↔human split

    // Inside human share, start from baseline (intentStrength vs 1-intentStrength)
    // and move toward "all manual" as manualBias → 1.
    // wManualHuman goes from (1-intentStrength) at bias=0 to 1 at bias=1
    float wManualHuman = ofLerp(1.0f - intentStrength, 1.0f, manualBias);
    float wIntentHuman = 1.0f - wManualHuman;

    wAuto   = agency;
    wManual = humanShare * wManualHuman;
    wIntent = humanShare * wIntentHuman;

    // Normalize
    float s = wAuto + wManual + wIntent;
    if (s > 1e-6f) { wAuto /= s; wManual /= s; wIntent /= s; }

    T targetValue = wAuto   * autoSmoothed
                  + wManual * manualSmoothed
                  + wIntent * intentSmoothed;

    value = smoothTo(value, targetValue, dt, targetSmoothSec);
  }
  
  T value;

private:
  ofParameter<T>& manualValueParameter;
  ofEventListener paramListener;
  float lastManualUpdateTime;
  
  T intentValue;
  T autoValue;
  
  float agency;
  float intentStrength;
  
  float baseManualBias { 0.1f }; // minimum human control share TODO: make this configurable as an ofParameter on the ofApp (`ofGetAppPtr()`)
  float manualBias { 0.0f }; // 1.0 after manual interaction, decays to baseManualBias
  float manualBiasDecaySec { 0.8f }; // time constant for manualBias decay
  
  float autoSmoothSec = 0.05f, intentSmoothSec = 0.25f, manualSmoothSec = 0.02f;
  T autoSmoothed, intentSmoothed, manualSmoothed;
  float targetSmoothSec = 0.3f;
};



} // namespace ofxMarkSynth
