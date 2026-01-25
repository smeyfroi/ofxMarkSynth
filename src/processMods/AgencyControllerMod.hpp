//
//  AgencyControllerMod.hpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 21/01/2026.
//

#pragma once

#include "../core/Mod.hpp"

#include <algorithm>

namespace ofxMarkSynth {

class AgencyControllerMod : public Mod {
public:
  AgencyControllerMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config);

  void update() override;
  void receive(int sinkId, const float& value) override;

  float getBudget() const { return budget; }
  float getStimulus() const { return stimulusSmooth; }
  float getAutoAgency() const { return autoAgency; }
  float getCharacteristicSmooth() const { return characteristicSmooth; }
  float getLastPulseDetectedValue() const { return lastPulseDetectedValue; }
  float getPulseThreshold() const { return pulseThresholdParameter.get(); }
  float getEventCost() const { return eventCostParameter.get(); }
  float getCooldownSec() const { return cooldownSecParameter.get(); }
  float getChargeGain() const { return chargeGainParameter.get(); }
  float getDecayPerSec() const { return decayPerSecParameter.get(); }
  float getLastDt() const { return lastDt; }
  float getLastChargeDelta() const { return lastChargeDelta; }
  float getLastDecayDelta() const { return lastDecayDelta; }
  float getSecondsSinceTrigger() const;
  float getSecondsSincePulseDetected() const;
  bool wasTriggeredThisFrame() const { return triggeredThisFrame; }
  bool wasPulseDetectedThisFrame() const { return pulseDetectedThisFrame; }
  bool didLastPulseTrigger() const { return lastPulseDidTrigger; }
  bool wasLastPulseBudgetEnough() const { return lastPulseBudgetEnough; }
  bool wasLastPulseCooldownOk() const { return lastPulseCooldownOk; }
  float getLastPulseBudget() const { return lastPulseBudget; }

  static constexpr int SINK_CHARACTERISTIC = 10;
  static constexpr int SINK_PULSE = 20;

  static constexpr int SOURCE_AUTO_AGENCY = 10;
  static constexpr int SOURCE_TRIGGER = 20;

protected:
  void initParameters() override;

private:
  float getDt() const;

  // Per-frame accumulators (combined with max)
  float characteristicMaxThisFrame { 0.0f };
  float pulseMaxThisFrame { 0.0f };

  float characteristicSmooth { 0.0f };
  float characteristicPrev { 0.0f };

  float stimulusSmooth { 0.0f };

  float budget { 0.0f };
  float autoAgency { 0.0f };
  float lastDt { 0.0f };
  float lastChargeDelta { 0.0f };
  float lastDecayDelta { 0.0f };

  float lastPulseDetectedValue { 0.0f };

  // For tuning: keep recent pulse/trigger status visible beyond one frame.
  bool pulseDetectedThisFrame { false };
  float lastPulseDetectedTimeSec { -1.0f };
  float lastPulseBudget { 0.0f };
  bool lastPulseBudgetEnough { false };
  bool lastPulseCooldownOk { false };
  bool lastPulseDidTrigger { false };

  bool triggeredThisFrame { false };
  float lastTriggerTimeSec { -1.0f };

  // Parameters
  ofParameter<float> characteristicSmoothSecParameter { "CharacteristicSmoothSec", 0.35f, 0.0f, 5.0f };
  ofParameter<float> stimulusSmoothSecParameter { "StimulusSmoothSec", 0.10f, 0.0f, 5.0f };

  ofParameter<float> chargeGainParameter { "ChargeGain", 2.0f, 0.0f, 50.0f };
  ofParameter<float> decayPerSecParameter { "DecayPerSec", 0.12f, 0.0f, 2.0f };

  ofParameter<float> autoAgencyScaleParameter { "AutoAgencyScale", 0.6f, 0.0f, 1.0f };
  ofParameter<float> autoAgencyGammaParameter { "AutoAgencyGamma", 0.7f, 0.1f, 3.0f };

  ofParameter<float> pulseThresholdParameter { "PulseThreshold", 0.8f, 0.0f, 1.0f };
  ofParameter<float> eventCostParameter { "EventCost", 0.20f, 0.0f, 1.0f };
  ofParameter<float> cooldownSecParameter { "CooldownSec", 1.5f, 0.0f, 10.0f };
};

} // namespace ofxMarkSynth
