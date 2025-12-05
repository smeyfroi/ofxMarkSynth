//
//  RandomFloatSourceMod.hpp
//  example_points
//
//  Created by Steve Meyfroidt on 05/05/2025.
//

#pragma once

#include "Mod.hpp"
#include "ParamController.h"

namespace ofxMarkSynth {


class RandomFloatSourceMod : public Mod {

public:
  RandomFloatSourceMod(Synth* synthPtr, const std::string& name, ModConfig config, std::pair<float, float> minRange = {0.0, 1.0}, std::pair<float, float> maxRange = {0.0, 1.0}, unsigned long randomSeed = 0);
  float getAgency() const override;
  void update() override;
  void applyIntent(const Intent& intent, float strength) override;
  
  static constexpr int SOURCE_FLOAT = 1;

protected:
  void initParameters() override;

private:
  float floatCount { 0.0f };
  ofParameter<float> floatsPerUpdateParameter { "CreatedPerUpdate", 1.0, 0.0, 100.0 };
  ParamController<float> floatsPerUpdateController { floatsPerUpdateParameter };
  ofParameter<float> minParameter { "Min", 0.0, 0.0, 1.0 }; // modified in ctor
  ParamController<float> minController { minParameter };
  ofParameter<float> maxParameter { "Max", 1.0, 0.0, 1.0 }; // modified in ctor
  ParamController<float> maxController { maxParameter };
  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0, 0.0, 1.0 };
  
  const float createRandomFloat() const;
};


} // ofxMarkSynth
