//
//  AgencyControllerMod.cpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 21/01/2026.
//

#include "AgencyControllerMod.hpp"

#include "ofAppRunner.h"
#include "ofMath.h"

#include <cmath>
#include <limits>

namespace ofxMarkSynth {

namespace {

float smoothTo(float current, float target, float dt, float timeConstantSec) {
  if (timeConstantSec <= 0.0f) return target;
  dt = std::max(dt, 0.0f);
  // Exponential decay: current += (target-current) * (1 - exp(-dt/tau))
  float a = 1.0f - std::exp(-dt / std::max(1e-6f, timeConstantSec));
  return current + (target - current) * a;
}

} // namespace

AgencyControllerMod::AgencyControllerMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  sinkNameIdMap = {
    { "Characteristic", SINK_CHARACTERISTIC },
    { "Pulse", SINK_PULSE },
  };

  sourceNameIdMap = {
    { "AutoAgency", SOURCE_AUTO_AGENCY },
    { "Trigger", SOURCE_TRIGGER },
  };
}

void AgencyControllerMod::initParameters() {
  parameters.add(characteristicSmoothSecParameter);
  parameters.add(stimulusSmoothSecParameter);

  parameters.add(chargeGainParameter);
  parameters.add(decayPerSecParameter);

  parameters.add(autoAgencyScaleParameter);
  parameters.add(autoAgencyGammaParameter);

  parameters.add(pulseThresholdParameter);
  parameters.add(eventCostParameter);
  parameters.add(cooldownSecParameter);
}

float AgencyControllerMod::getDt() const {
  // Note: Synth caps dt for time tracking; we accept small inconsistencies here.
  float frameTime = static_cast<float>(ofGetLastFrameTime());
  return std::min(std::max(frameTime, 0.0f), 0.1f);
}

void AgencyControllerMod::update() {
  syncControllerAgencies();
  triggeredThisFrame = false;
  pulseDetectedThisFrame = false;

  float dt = getDt();
  lastDt = dt;

  // 1) Smooth characteristic and compute stimulus
  float characteristicRaw = std::clamp(characteristicMaxThisFrame, 0.0f, 1.0f);
  characteristicMaxThisFrame = 0.0f;

  characteristicSmooth = smoothTo(characteristicSmooth, characteristicRaw, dt, characteristicSmoothSecParameter);

  float stimulusRaw = std::abs(characteristicSmooth - characteristicPrev);
  characteristicPrev = characteristicSmooth;

  stimulusSmooth = smoothTo(stimulusSmooth, stimulusRaw, dt, stimulusSmoothSecParameter);

  // 2) Budget integrates stimulus, decays slowly
  // Note: stimulus is derived from per-frame delta of a smoothed signal.
  // ChargeGain is therefore effectively a scale on |Δ characteristic|.
  lastChargeDelta = chargeGainParameter * stimulusSmooth;
  lastDecayDelta = decayPerSecParameter * dt;
  budget += lastChargeDelta;
  budget -= lastDecayDelta;
  budget = std::clamp(budget, 0.0f, 1.0f);

  // 3) Convert budget to auto agency
  float gamma = std::max(0.1f, static_cast<float>(autoAgencyGammaParameter));
  autoAgency = autoAgencyScaleParameter * std::pow(budget, gamma);
  autoAgency = std::clamp(autoAgency, 0.0f, 1.0f);

  emit(SOURCE_AUTO_AGENCY, autoAgency);

  // 4) Event gating from pulses
  bool shouldTrigger = false;
  float pulse = pulseMaxThisFrame;
  pulseMaxThisFrame = 0.0f;

  float now = ofGetElapsedTimef();
  float pulseThreshold = pulseThresholdParameter;
  float eventCost = eventCostParameter;
  float cooldownSec = cooldownSecParameter;
  bool pulseDetected = pulse > pulseThreshold;

  // Track pulse state even if it fails gating (for tuning visibility).
  if (pulseDetected) {
    pulseDetectedThisFrame = true;
    lastPulseDetectedTimeSec = now;
    lastPulseDetectedValue = pulse;
    lastPulseBudget = budget;
    lastPulseBudgetEnough = budget >= eventCost;

    float sinceTrigger = (lastTriggerTimeSec < 0.0f) ? std::numeric_limits<float>::infinity() : (now - lastTriggerTimeSec);
    lastPulseCooldownOk = sinceTrigger >= cooldownSec;
    lastPulseDidTrigger = false;
  }

  if (pulseDetected && budget >= eventCost) {
    bool cooldownOk = (lastTriggerTimeSec < 0.0f) || (now - lastTriggerTimeSec >= cooldownSec);
    if (cooldownOk) {
      shouldTrigger = true;
      lastTriggerTimeSec = now;
      budget = std::max(0.0f, budget - eventCost);
      lastPulseDidTrigger = true;
    }
  }

  if (shouldTrigger) {
    triggeredThisFrame = true;
    emit(SOURCE_TRIGGER, 1.0f);
  }
}

void AgencyControllerMod::receive(int sinkId, const float& value) {
  switch (sinkId) {
    case SINK_CHARACTERISTIC:
      characteristicMaxThisFrame = std::max(characteristicMaxThisFrame, value);
      break;

    case SINK_PULSE:
      pulseMaxThisFrame = std::max(pulseMaxThisFrame, value);
      break;

    default:
      ofLogError("AgencyControllerMod") << "Float receive for unknown sinkId " << sinkId;
  }
}

float AgencyControllerMod::getSecondsSinceTrigger() const {
  if (lastTriggerTimeSec < 0.0f) return std::numeric_limits<float>::infinity();
  return ofGetElapsedTimef() - lastTriggerTimeSec;
}

float AgencyControllerMod::getSecondsSincePulseDetected() const {
  if (lastPulseDetectedTimeSec < 0.0f) return std::numeric_limits<float>::infinity();
  return ofGetElapsedTimef() - lastPulseDetectedTimeSec;
}

} // namespace ofxMarkSynth
