//
//  FluidRadialImpulseMod.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 25/05/2025.
//

#pragma once

#include "Mod.hpp"
#include "AddRadialImpulseShader.h"


namespace ofxMarkSynth {


class FluidRadialImpulseMod : public Mod {

public:
  FluidRadialImpulseMod(Synth* synthPtr, const std::string& name, const ModConfig&& config);
  void update() override;
  void receive(int sinkId, const float& value) override;
  void receive(int sinkId, const glm::vec2& point) override;

  static constexpr int SINK_POINTS = 1;
  static constexpr int SINK_IMPULSE_RADIUS = 10;
  static constexpr int SINK_IMPULSE_STRENGTH = 20;

protected:
  void initParameters() override;

private:
  ofParameter<float> impulseRadiusParameter { "ImpulseRadius", 0.001, 0.0, 0.3 };
  ofParameter<float> impulseStrengthParameter { "ImpulseStrength", 0.01, 0.0, 0.3 };

  std::vector<glm::vec2> newPoints;
  
  AddRadialImpulseShader addRadialImpulseShader;
};


} // ofxMarkSynth
