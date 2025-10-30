//
//  RandomVecSourceMod.hpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#pragma once

#include "Mod.hpp"
#include "IntentParamController.h"

namespace ofxMarkSynth {


class RandomVecSourceMod : public Mod {

public:
  RandomVecSourceMod(Synth* synthPtr, const std::string& name, const ModConfig&& config, short vecDimensions);
  void update() override;
  void applyIntent(const Intent& intent, float strength) override;
  
  static constexpr int SOURCE_VEC2 = 2;
  static constexpr int SOURCE_VEC3 = 3;
  static constexpr int SOURCE_VEC4 = 4;

protected:
  void initParameters() override;

private:
  short vecDimensions;
  
  float vecCount;
  ofParameter<float> vecsPerUpdateParameter { "CreatedPerUpdate", 1.0, 0.0, 100.0 };
  IntentParamController<float> vecsPerUpdateController { vecsPerUpdateParameter };
  
  const glm::vec2 createRandomVec2() const;
  const glm::vec3 createRandomVec3() const;
  const glm::vec4 createRandomVec4() const;
};


} // ofxMarkSynth
