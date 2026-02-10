#pragma once

#include "ofMain.h"
#include "util/Lerp.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
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
  std::string intentMappingDescription;  // e.g., "E×G → exp(2)"
  virtual void setAgency(float a) = 0; // ensure GUI reads controller-computed weights
  virtual void syncWithParameter() = 0; // sync controller value with parameter (after config load)
  virtual std::string getFormattedValue() const = 0; // formatted string showing component breakdown and final value
};



// Helper functions to format different value types as strings
namespace ParamFormat {

inline std::string formatSingleValue(int v) {
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%d", v);
  return buf;
}

inline std::string formatSingleValue(float v) {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%.4f", v);
  return buf;
}

inline std::string formatSingleValue(const glm::vec2& v) {
  char buf[64];
  std::snprintf(buf, sizeof(buf), "(%.3f, %.3f)", v.x, v.y);
  return buf;
}

inline std::string formatSingleValue(const glm::vec4& v) {
  char buf[96];
  std::snprintf(buf, sizeof(buf), "(%.3f, %.3f, %.3f, %.3f)", v.x, v.y, v.z, v.w);
  return buf;
}

inline std::string formatSingleValue(const ofFloatColor& c) {
  char buf[96];
  std::snprintf(buf, sizeof(buf), "RGBA(%.2f, %.2f, %.2f, %.2f)", c.r, c.g, c.b, c.a);
  return buf;
}

// Fallback for other types
template<typename T>
inline std::string formatSingleValue(const T& v) {
  return "?";
}

} // namespace ParamFormat



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

  T getManualValue() const {
    return manualValueParameter.get();
  }
  
  float getTimeSinceLastManualUpdate() const {
    return ofGetElapsedTimef() - lastManualUpdateTime;
  }
  
  bool isManualControlActive(float thresholdTime = 0.5) const {
    return getTimeSinceLastManualUpdate() < thresholdTime;
  }
  
  void updateIntent(T newIntentValue, float newIntentStrength, 
                    const std::string& mappingDesc = "") {
    intentValue = clampToManualRange(newIntentValue);
    intentStrength = newIntentStrength;
    hasReceivedIntentValue = true;
    if (!mappingDesc.empty()) {
      intentMappingDescription = mappingDesc;
    }
    update();
  }
  
  void updateAuto(T newAutoValue, float newAgency) {
    autoValue = clampToManualRange(newAutoValue);
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
  
  // Return formatted string showing component breakdown and final value (for tooltip)
  std::string getFormattedValue() const override {
    std::string result;
    char buf[32];
    
    if (hasReceivedAutoValue && wAuto > 0.005f) {
      std::snprintf(buf, sizeof(buf), "Auto (%.0f%%): ", wAuto * 100.0f);
      result += buf;
      result += ParamFormat::formatSingleValue(autoSmoothed);
      result += "\n";
    }
    
    if (hasReceivedIntentValue && wIntent > 0.005f) {
      std::snprintf(buf, sizeof(buf), "Intent (%.0f%%): ", wIntent * 100.0f);
      result += buf;
      result += ParamFormat::formatSingleValue(intentSmoothed);
      if (!intentMappingDescription.empty()) {
        result += "\n  = ";
        result += intentMappingDescription;
      }
      result += "\n";
    }
    
    if (wManual > 0.005f) {
      std::snprintf(buf, sizeof(buf), "Manual (%.0f%%): ", wManual * 100.0f);
      result += buf;
      result += ParamFormat::formatSingleValue(manualSmoothed);
      result += "\n";
    }
    
    result += "----------------\n";
    result += "Final: ";
    result += ParamFormat::formatSingleValue(value);
    
    return result;
  }
  
  void update() {
    float dt = ofGetLastFrameTime();
    
    // Use global settings for manual bias decay behavior
    auto& settings = ParamControllerSettings::instance();
    manualBias = isManualControlActive() ? 1.0f : smoothToFloat(manualBias, settings.baseManualBias, dt, settings.manualBiasDecaySec);
    
    // Use angular or linear smoothing based on the angular flag
    // For int types, don't smooth - just snap to target values
    if constexpr (std::is_same_v<T, int>) {
      manualSmoothed = manualValueParameter.get();
      autoSmoothed   = autoValue;
      intentSmoothed = intentValue;
    } else if constexpr (std::is_same_v<T, float>) {
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
    } else if constexpr (std::is_same_v<T, ofFloatColor>) {
      // ofFloatColor's arithmetic operators ignore alpha, so use explicit weighted blend
      targetValue = weightedBlend(autoSmoothed, wAuto, manualSmoothed, wManual, intentSmoothed, wIntent);
    } else if constexpr (std::is_same_v<T, int>) {
      // Discrete int values don't interpolate well - use winner-takes-all
      // Pick the value from the source with the highest weight
      if (wManual >= wAuto && wManual >= wIntent) {
        targetValue = manualSmoothed;
      } else if (wIntent >= wAuto) {
        targetValue = intentSmoothed;
      } else {
        targetValue = autoSmoothed;
      }
    } else {
      targetValue = wAuto * autoSmoothed + wManual * manualSmoothed + wIntent * intentSmoothed;
    }

    targetValue = clampToManualRange(targetValue);
    
    // Final smoothing to target
    // For int types, don't smooth - just snap to target
    if constexpr (std::is_same_v<T, int>) {
      value = targetValue;
    } else if constexpr (std::is_same_v<T, float>) {
      if (angular) {
        value = smoothToAngular(value, targetValue, dt, targetSmoothSec);
      } else {
        value = smoothTo(value, targetValue, dt, targetSmoothSec);
      }
    } else {
      value = smoothTo(value, targetValue, dt, targetSmoothSec);
    }

    value = clampToManualRange(value);
  }
  
  T value;
  
private:
  T clampToManualRange(const T& v) const {
    if constexpr (std::is_same_v<T, float>) {
      return ofClamp(v, manualValueParameter.getMin(), manualValueParameter.getMax());
    } else if constexpr (std::is_same_v<T, int>) {
      return std::clamp(v, manualValueParameter.getMin(), manualValueParameter.getMax());
    } else if constexpr (std::is_same_v<T, glm::vec2>) {
      const auto mn = manualValueParameter.getMin();
      const auto mx = manualValueParameter.getMax();
      return { ofClamp(v.x, mn.x, mx.x), ofClamp(v.y, mn.y, mx.y) };
    } else if constexpr (std::is_same_v<T, glm::vec4>) {
      const auto mn = manualValueParameter.getMin();
      const auto mx = manualValueParameter.getMax();
      return { ofClamp(v.x, mn.x, mx.x), ofClamp(v.y, mn.y, mx.y), ofClamp(v.z, mn.z, mx.z), ofClamp(v.w, mn.w, mx.w) };
    } else if constexpr (std::is_same_v<T, ofFloatColor>) {
      const auto mn = manualValueParameter.getMin();
      const auto mx = manualValueParameter.getMax();
      return { ofClamp(v.r, mn.r, mx.r), ofClamp(v.g, mn.g, mx.g), ofClamp(v.b, mn.b, mx.b), ofClamp(v.a, mn.a, mx.a) };
    } else {
      return v;
    }
  }

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
