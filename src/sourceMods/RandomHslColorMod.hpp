//
//  RandomHslColorMod.hpp
//  example_sandlines
//
//  Created by Steve Meyfroidt on 10/05/2025.
//

#pragma once

#include "Mod.hpp"
#include "IntentParamController.h"

namespace ofxMarkSynth {


class RandomHslColorMod : public Mod {

public:
  RandomHslColorMod(Synth* synthPtr, const std::string& name, const ModConfig&& config);
  void update() override;
  void applyIntent(const Intent& intent, float strength) override;
  
  static constexpr int SOURCE_VEC4 = 1;

protected:
  void initParameters() override;

private:
  float colorCount;
  const ofFloatColor createRandomColor() const;
  ofParameter<float> colorsPerUpdateParameter { "CreatedPerUpdate", 1.0, 0.0, 100.0 };
  IntentParamController<float> colorsPerUpdateController { colorsPerUpdateParameter };
  ofParameter<float> minSaturationParameter { "MinSaturation", 0.0, 0.0, 1.0 };
  IntentParamController<float> minSaturationController { minSaturationParameter };
  ofParameter<float> maxSaturationParameter { "MaxSaturation", 1.0, 0.0, 1.0 };
  IntentParamController<float> maxSaturationController { maxSaturationParameter };
  ofParameter<float> minLightnessParameter { "MinLightness", 0.0, 0.0, 1.0 };
  IntentParamController<float> minLightnessController { minLightnessParameter };
  ofParameter<float> maxLightnessParameter { "MaxLightness", 1.0, 0.0, 1.0 };
  IntentParamController<float> maxLightnessController { maxLightnessParameter };
  ofParameter<float> minAlphaParameter { "MinAlpha", 0.0, 0.0, 1.0 };
  IntentParamController<float> minAlphaController { minAlphaParameter };
  ofParameter<float> maxAlphaParameter { "MaxAlpha", 1.0, 0.0, 1.0 };
  IntentParamController<float> maxAlphaController { maxAlphaParameter };
};


} // ofxMarkSynth
