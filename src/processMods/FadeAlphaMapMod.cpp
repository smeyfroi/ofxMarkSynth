//
//  FadeAlphaMapMod.cpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 02/02/2026.
//

#include "FadeAlphaMapMod.hpp"

#include <algorithm>
#include <cmath>

namespace ofxMarkSynth {

FadeAlphaMapMod::FadeAlphaMapMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config)
: Mod { std::move(synthPtr), name, std::move(config) }
{
  sinkNameIdMap = {
    { multiplierParameter.getName(), SINK_MULTIPLIER },
    { adderParameter.getName(), SINK_ADDER },
    { "float", SINK_FLOAT }
  };

  sourceNameIdMap = {
    { "float", SOURCE_FLOAT }
  };

  registerControllerForSource(multiplierParameter, multiplierController);
  registerControllerForSource(adderParameter, adderController);
}

void FadeAlphaMapMod::initParameters() {
  parameters.add(multiplierParameter);
  parameters.add(adderParameter);
  parameters.add(referenceFpsParameter);
  parameters.add(minHalfLifeSecParameter);
  parameters.add(maxHalfLifeSecParameter);
  parameters.add(agencyFactorParameter);
}

float FadeAlphaMapMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void FadeAlphaMapMod::update() {
  syncControllerAgencies();
  multiplierController.update();
  adderController.update();
}

float FadeAlphaMapMod::mapAlphaToHalfLifeSec(float alphaPerFrame) const {
  float minHalfLifeSec = std::max(1e-6f, static_cast<float>(minHalfLifeSecParameter));
  float maxHalfLifeSec = std::max(minHalfLifeSec, static_cast<float>(maxHalfLifeSecParameter));
  float fps = std::max(1e-3f, static_cast<float>(referenceFpsParameter));

  // Interpret alphaPerFrame as fraction removed per frame at `fps`.
  // Remaining multiplier per frame = (1 - alpha).
  float alpha = std::clamp(alphaPerFrame, 0.0f, 1.0f - 1e-6f);
  if (alpha <= 0.0f) return maxHalfLifeSec;

  float logRemain = std::log1p(-alpha); // log(1 - alpha)
  if (!std::isfinite(logRemain) || logRemain >= 0.0f) return maxHalfLifeSec;

  float halfLifeFrames = std::log(0.5f) / logRemain;
  float halfLifeSec = halfLifeFrames / fps;
  if (!std::isfinite(halfLifeSec)) return maxHalfLifeSec;

  return std::clamp(halfLifeSec, minHalfLifeSec, maxHalfLifeSec);
}

void FadeAlphaMapMod::receive(int sinkId, const float& value) {
  switch (sinkId) {
    case SINK_MULTIPLIER:
      multiplierController.updateAuto(value, getAgency());
      break;
    case SINK_ADDER:
      adderController.updateAuto(value, getAgency());
      break;
    case SINK_FLOAT: {
      float alphaPerFrame = value * multiplierController.value + adderController.value;
      float halfLifeSec = mapAlphaToHalfLifeSec(alphaPerFrame);
      emit(SOURCE_FLOAT, halfLifeSec);
      break;
    }
    default:
      ofLogError("FadeAlphaMapMod") << "Float receive for unknown sinkId " << sinkId;
  }
}

} // namespace ofxMarkSynth
