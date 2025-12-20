//
//  FluidRadialImpulseMod.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 25/05/2025.
//

#pragma once

#include <vector>
#include "Mod.hpp"
#include "AddRadialImpulseShader.h"
#include "ParamController.h"


namespace ofxMarkSynth {


class FluidRadialImpulseMod : public Mod {

public:
  FluidRadialImpulseMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config);
  float getAgency() const override;
  void update() override;
  void receive(int sinkId, const float& value) override;
  void receive(int sinkId, const glm::vec2& point) override;
  void applyIntent(const Intent& intent, float strength) override;

  static constexpr int SINK_POINTS = 1;
  static constexpr int SINK_IMPULSE_RADIUS = 10;
  static constexpr int SINK_IMPULSE_STRENGTH = 20;

protected:
  void initParameters() override;

private:
  ofParameter<float> impulseRadiusParameter { "Impulse Radius", 0.01, 0.0, 0.15 };
  ParamController<float> impulseRadiusController { impulseRadiusParameter };
  ofParameter<float> impulseStrengthParameter { "Impulse Strength", 0.5, 0.0, 1.5 };
  ParamController<float> impulseStrengthController { impulseStrengthParameter };
  ofParameter<float> dtParameter { "dt", 0.1, 0.001, 1.0 };
  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0, 0.0, 1.0 };

  std::vector<glm::vec2> newPoints;
  
  AddRadialImpulseShader addRadialImpulseShader;
};


} // ofxMarkSynth
