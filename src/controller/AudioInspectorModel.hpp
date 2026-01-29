#pragma once

#include <array>
#include <optional>
#include <string>

#include "ofxAudioAnalysisClient.h"



namespace ofxMarkSynth {



class AudioInspectorModel {
public:
  struct ScalarStats {
    ofxAudioAnalysisClient::AnalysisScalar scalar;
    std::string label;
    float rawValue = 0.0f;         // filtered (matching synth output)
    float minValue = 0.0f;         // configured min (real units)
    float maxValue = 1.0f;         // configured max (real units)
    float unwrapped = 0.0f;        // u = (raw-min)/(max-min)
    float wrapped = 0.0f;          // w = frac(abs(u))
    float outLowPct = 0.0f;        // EMA of u<0 (0..100)
    float outHighPct = 0.0f;       // EMA of u>1 (0..100)
  };

  struct DetectorStats {
    std::string label;
    float zScore = 0.0f;
    float threshold = 0.0f;
    float cooldownRemaining = 0.0f;
    float cooldownTotal = 0.0f;
  };

  static constexpr float DEFAULT_WINDOW_SECONDS = 15.0f;

  void reset();

  // Update rolling stats for one scalar.
  // `dtSeconds` should represent time since last new audio update.
  ScalarStats updateScalar(const ScalarStats& input, float dtSeconds, float windowSeconds = DEFAULT_WINDOW_SECONDS);

  static float computeUnwrapped(float value, float minValue, float maxValue);
  static float wrap01(float value);

private:
  struct ScalarEmaState {
    float outLowEma = 0.0f;   // 0..1
    float outHighEma = 0.0f;  // 0..1
  };

  std::array<ScalarEmaState, static_cast<int>(ofxAudioAnalysisClient::AnalysisScalar::_count)> emaStates {};
};



} // namespace ofxMarkSynth
