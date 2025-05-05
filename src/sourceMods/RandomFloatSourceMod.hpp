//
//  RandomFloatSourceMod.hpp
//  example_points
//
//  Created by Steve Meyfroidt on 05/05/2025.
//

#pragma once

#include "Mod.hpp"

namespace ofxMarkSynth {


class RandomFloatSourceMod : public Mod {

public:
  RandomFloatSourceMod(const std::string& name, const ModConfig&& config, std::pair<float, float> minRange = {0.0, 1.0}, std::pair<float, float> maxRange = {0.0, 1.0}, unsigned long randomSeed = 0);
  void update() override;
  
  static constexpr int SOURCE_FLOAT = 1;

protected:
  void initParameters() override;

private:
  float floatCount;
  ofParameter<float> floatsPerUpdateParameter { "CreatedPerUpdate", 1.0, 0.0, 100.0 };
  ofParameter<float> minParameter { "MinRadius", 0.0, 0.0, 1.0 }; // modified in ctor
  ofParameter<float> maxParameter { "MaxRadius", 1.0, 0.0, 1.0 }; // modified in ctor
  
  const float createRandomFloat() const;
};


} // ofxMarkSynth
