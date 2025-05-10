//
//  RandomHslColorMod.hpp
//  example_sandlines
//
//  Created by Steve Meyfroidt on 10/05/2025.
//

#pragma once

#include "Mod.hpp"

namespace ofxMarkSynth {


class RandomHslColorMod : public Mod {

public:
  RandomHslColorMod(const std::string& name, const ModConfig&& config);
  void update() override;
  
  static constexpr int SOURCE_VEC4 = 1;

protected:
  void initParameters() override;

private:
  float colorCount;
  const ofFloatColor createRandomColor() const;
  ofParameter<float> colorsPerUpdateParameter { "CreatedPerUpdate", 1.0, 0.0, 100.0 };
  ofParameter<float> minSaturationParameter { "MinSaturation", 0.0, 0.0, 1.0 };
  ofParameter<float> maxSaturationParameter { "MaxSaturation", 1.0, 0.0, 1.0 };
  ofParameter<float> minLightnessParameter { "MinLightness", 0.0, 0.0, 1.0 };
  ofParameter<float> maxLightnessParameter { "MaxLightness", 1.0, 0.0, 1.0 };
  ofParameter<float> minAlphaParameter { "MinAlpha", 0.0, 0.0, 1.0 };
  ofParameter<float> maxAlphaParameter { "MaxAlpha", 1.0, 0.0, 1.0 };
};


} // ofxMarkSynth
