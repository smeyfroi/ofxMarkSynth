#include "controller/AudioInspectorModel.hpp"

#include <algorithm>
#include <cmath>



namespace ofxMarkSynth {



void AudioInspectorModel::reset() {
  for (auto& s : emaStates) {
    s.outLowEma = 0.0f;
    s.outHighEma = 0.0f;
  }
}

float AudioInspectorModel::computeUnwrapped(float value, float minValue, float maxValue) {
  float range = maxValue - minValue;
  if (std::abs(range) < 1e-9f) {
    return 0.0f;
  }
  return (value - minValue) / range;
}

float AudioInspectorModel::wrap01(float value) {
  value = std::abs(value);
  return value - std::floor(value);
}

AudioInspectorModel::ScalarStats AudioInspectorModel::updateScalar(const ScalarStats& input,
                                                                   float dtSeconds,
                                                                   float windowSeconds) {
  ScalarStats out = input;

  out.unwrapped = computeUnwrapped(out.rawValue, out.minValue, out.maxValue);
  out.wrapped = wrap01(out.unwrapped);

  int idx = static_cast<int>(out.scalar);
  if (idx < 0 || idx >= static_cast<int>(emaStates.size())) {
    return out;
  }

  // Exponential moving average approximating a rolling window.
  float safeWindow = std::max(0.001f, windowSeconds);
  float alpha = std::clamp(dtSeconds / safeWindow, 0.0f, 1.0f);

  float lowSample = (out.unwrapped < 0.0f) ? 1.0f : 0.0f;
  float highSample = (out.unwrapped > 1.0f) ? 1.0f : 0.0f;

  emaStates[idx].outLowEma = emaStates[idx].outLowEma + alpha * (lowSample - emaStates[idx].outLowEma);
  emaStates[idx].outHighEma = emaStates[idx].outHighEma + alpha * (highSample - emaStates[idx].outHighEma);

  out.outLowPct = emaStates[idx].outLowEma * 100.0f;
  out.outHighPct = emaStates[idx].outHighEma * 100.0f;

  return out;
}



} // namespace ofxMarkSynth
