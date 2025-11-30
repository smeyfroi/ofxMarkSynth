#pragma once

#include "ofMain.h"
#include "util/Lerp.h"
#include <cmath>
#include <type_traits>

// Exponential smoothing toward target over timeConstant seconds
inline float smoothToFloat(float current, float target, float dt, float timeConstant) {
  if (timeConstant <= 0.0f) return target;
  float alpha = 1.0f - std::exp(-dt / timeConstant);
  return ofLerp(current, target, alpha);
}

// Angular smoothing for cyclic values (e.g., hue in [0, 1])
// Takes the shortest path around the circle
inline float smoothToAngular(float current, float target, float dt, float timeConstant) {
  if (timeConstant <= 0.0f) return target;
  float alpha = 1.0f - std::exp(-dt / timeConstant);
  return lerpAngular(current, target, alpha);
}

template<typename T>
inline T smoothTo(const T& current, const T& target, float dt, float timeConstant) {
  if (timeConstant <= 0.0f) return target;
  float alpha = 1.0f - std::exp(-dt / timeConstant);
  return lerp(current, target, alpha);
}



namespace ofxMarkSynth {



// Global settings for all ParamControllers - set by Synth, read by ParamControllers
struct ParamControllerSettings {
  float manualBiasDecaySec = 0.8f;  // Time constant for manual bias decay
  float baseManualBias = 0.1f;      // Minimum manual control share (doesn't fully decay to zero)
  
  static ParamControllerSettings& instance() {
    static ParamControllerSettings settings;
    return settings;
  }
};



// This only exists so that we can introspect the weights for Gui without needing to templatize everything
class BaseParamController {
public:
  virtual ~BaseParamController() = default;
  float wAuto = 0.0f;
  float wManual = 1.0f;
  float wIntent = 0.0f;
  bool hasReceivedAutoValue = false;
  bool hasReceivedIntentValue = false;
  virtual void setAgency(float a) = 0; // ensure GUI reads controller-computed weights
  virtual void syncWithParameter() = 0; // sync controller value with parameter (after config load)
};



template<typename T>
class ParamController : public BaseParamController {
public:
  ParamController(ofParameter<T>& manualValueParameter_, bool isAngular_ = false)
  : manualValueParameter(manualValueParameter_),
  value(manualValueParameter_.get()),
  intentValue(manualValueParameter_.get()),
  autoValue(manualValueParameter_.get()),
  autoSmoothed(manualValueParameter_.get()),
  intentSmoothed(manualValueParameter_.get()),
  manualSmoothed(manualValueParameter_.get()),
  agency(0.0f),
  intentStrength(0.0f),
  lastManualUpdateTime(0.0f),
  angular(isAngular_)
  {
    paramListener = manualValueParameter.newListener([this](T&) {
      lastManualUpdateTime = ofGetElapsedTimef();
    });
    
    // Initialize weights (wAuto, wManual, wIntent) before first GUI render
    update();
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
  
  bool isManualControlActive(float thresholdTime = 0.5) const {
    return getTimeSinceLastManualUpdate() < thresholdTime;
  }
  
  void updateIntent(T newIntentValue, float newIntentStrength) {
    intentValue = newIntentValue;
    intentStrength = newIntentStrength;
    hasReceivedIntentValue = true;
    update();
  }
  
  void updateAuto(T newAutoValue, float newAgency) {
    autoValue = newAutoValue;
    agency = newAgency;
    hasReceivedAutoValue = true;
    update();
  }
  
  // Allow Mods to push their live agency so GUI uses controller-computed weights
  void setAgency(float a) override {
    agency = a;
  }
  
  // Sync controller value with parameter value (called after config load)
  void syncWithParameter() override {
    T paramValue = manualValueParameter.get();
    value = paramValue;
    manualSmoothed = paramValue;
    autoSmoothed = paramValue;
    intentSmoothed = paramValue;
    autoValue = paramValue;
    intentValue = paramValue;
  }
  
  void update() {
    float dt = ofGetLastFrameTime();
    
    // Use global settings for manual bias decay behavior
    auto& settings = ParamControllerSettings::instance();
    manualBias = isManualControlActive() ? 1.0f : smoothToFloat(manualBias, settings.baseManualBias, dt, settings.manualBiasDecaySec);
    
    // Use angular or linear smoothing based on the angular flag
    if constexpr (std::is_same_v<T, float>) {
      if (angular) {
        manualSmoothed = smoothToAngular(manualSmoothed, manualValueParameter.get(), dt, manualSmoothSec);
        autoSmoothed   = smoothToAngular(autoSmoothed, autoValue, dt, autoSmoothSec);
        intentSmoothed = smoothToAngular(intentSmoothed, intentValue, dt, intentSmoothSec);
      } else {
        manualSmoothed = smoothTo(manualSmoothed, manualValueParameter.get(), dt, manualSmoothSec);
        autoSmoothed   = smoothTo(autoSmoothed, autoValue, dt, autoSmoothSec);
        intentSmoothed = smoothTo(intentSmoothed, intentValue, dt, intentSmoothSec);
      }
    } else {
      manualSmoothed = smoothTo(manualSmoothed, manualValueParameter.get(), dt, manualSmoothSec);
      autoSmoothed   = smoothTo(autoSmoothed, autoValue, dt, autoSmoothSec);
      intentSmoothed = smoothTo(intentSmoothed, intentValue, dt, intentSmoothSec);
    }
    
    // --- Weighting: outer (auto vs human), inner (manual vs intent) ---
    // Only use auto weight if we've actually received auto values from a connection.
    // Otherwise, redistribute to manual/intent so unconnected parameters stay under manual control.
    float effectiveAgency = hasReceivedAutoValue ? agency : 0.0f;
    float humanShare = 1.0f - effectiveAgency;
    
    // Inside human share, start from baseline (intentStrength vs 1-intentStrength)
    // and move toward "all manual" as manualBias → 1.
    // wManualHuman goes from (1-intentStrength) at bias=0 to 1 at bias=1
    // Only use intent weight if we've actually received intent values.
    float effectiveIntentStrength = hasReceivedIntentValue ? intentStrength : 0.0f;
    float wManualHuman = ofLerp(1.0f - effectiveIntentStrength, 1.0f, manualBias);
    float wIntentHuman = 1.0f - wManualHuman;
    
    wAuto   = effectiveAgency;
    wManual = humanShare * wManualHuman;
    wIntent = humanShare * wIntentHuman;
    
    // Normalize
    float s = wAuto + wManual + wIntent;
    if (s > 1e-6f) { wAuto /= s; wManual /= s; wIntent /= s; }
    
    // Blend values using angular or linear interpolation
    T targetValue;
    if constexpr (std::is_same_v<T, float>) {
      if (angular) {
        // For angular values, we need to blend taking the circular nature into account
        // First blend auto and manual
        float autoManualBlend = lerpAngular(autoSmoothed, manualSmoothed, wManual / (wAuto + wManual + 1e-6f));
        // Then blend with intent
        float totalAutoManual = wAuto + wManual;
        targetValue = lerpAngular(autoManualBlend, intentSmoothed, wIntent / (totalAutoManual + wIntent + 1e-6f));
      } else {
        targetValue = wAuto * autoSmoothed + wManual * manualSmoothed + wIntent * intentSmoothed;
      }
    } else {
      targetValue = wAuto * autoSmoothed + wManual * manualSmoothed + wIntent * intentSmoothed;
    }
    
    // Final smoothing to target
    if constexpr (std::is_same_v<T, float>) {
      if (angular) {
        value = smoothToAngular(value, targetValue, dt, targetSmoothSec);
      } else {
        value = smoothTo(value, targetValue, dt, targetSmoothSec);
      }
    } else {
      value = smoothTo(value, targetValue, dt, targetSmoothSec);
    }
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
  
  // Note: baseManualBias and manualBiasDecaySec are now in ParamControllerSettings global singleton
  float manualBias { 0.0f }; // 1.0 after manual interaction, decays to baseManualBias (from settings)
  
  float autoSmoothSec = 0.05f, intentSmoothSec = 0.25f, manualSmoothSec = 0.02f;
  T autoSmoothed, intentSmoothed, manualSmoothed;
  float targetSmoothSec = 0.3f;
  
  bool angular = false; // For cyclic values like hue (only meaningful for float)
};



} // namespace ofxMarkSynth
