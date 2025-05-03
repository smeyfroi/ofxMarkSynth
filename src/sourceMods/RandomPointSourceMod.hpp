//
//  RandomPointSourceMod.hpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#pragma once

#include "Mod.hpp"

namespace ofxMarkSynth {


class RandomPointSourceMod : public Mod {

public:
  RandomPointSourceMod(const std::string& name, const ModConfig&& config);
  void update() override;
  
  static constexpr int SOURCE_POINTS = 1;

protected:
  void initParameters() override;

private:
  float pointCount;
  ofParameter<float> pointsPerUpdateParameter { "PointsPerUpdate", 1.0, 0.0, 100.0 };
  
  const glm::vec2 createRandomPoint() const;
};


} // ofxMarkSynth
