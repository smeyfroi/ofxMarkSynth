//
//  FluidRadialImpulseMod.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 25/05/2025.
//

#pragma once

#include <vector>
#include "../core/Mod.hpp"
#include "AddRadialImpulseShader.h"
#include "../core/ParamController.h"


namespace ofxMarkSynth {


class FluidRadialImpulseMod : public Mod {

public:
  FluidRadialImpulseMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config);
  float getAgency() const override;
  void update() override;
  void receive(int sinkId, const float& value) override;
  void receive(int sinkId, const glm::vec2& point) override;
  void receive(int sinkId, const glm::vec4& pointVelocity) override;
  void applyIntent(const Intent& intent, float strength) override;

  static constexpr int SINK_POINTS = 1;
  static constexpr int SINK_POINT_VELOCITY = 2;
  static constexpr int SINK_VELOCITY = 3;
  static constexpr int SINK_SWIRL_VELOCITY = 4;
  static constexpr int SINK_IMPULSE_RADIUS = 10;
  static constexpr int SINK_IMPULSE_STRENGTH = 20;

protected:
  void initParameters() override;

private:
  ofParameter<float> impulseRadiusParameter { "Impulse Radius", 0.01, 0.0, 0.10 };
  ParamController<float> impulseRadiusController { impulseRadiusParameter };
  ofParameter<float> impulseStrengthParameter { "Impulse Strength", 0.5, 0.0, 1.0 };
  ParamController<float> impulseStrengthController { impulseStrengthParameter };
  // Interpreted as the dt used by the impulse injection shader (must match the fluid solver's dt semantics).
  ofParameter<float> dtParameter { "dt", 0.0015f, 0.0f, 0.003f };

  // Scales incoming normalized velocity sinks to pixel displacement per step.
  // For a W×H velocity buffer: px = VelocityScale * (dxNorm*W, dyNorm*H)
  ofParameter<float> velocityScaleParameter { "VelocityScale", 1.0f, 0.0f, 50.0f };

  // Multiplier for SwirlVelocity (0..1). Config/manual only (no controller).
  // Effective swirl = clamp(SwirlVelocity * SwirlStrength, 0..1).
  ofParameter<float> swirlStrengthParameter { "SwirlStrength", 1.0f, 0.0f, 2.0f };

  // Additional normalized swirl term that can be set from config OR driven via the SwirlVelocity sink.
  ofParameter<float> swirlVelocityParameter { "SwirlVelocity", 0.0f, 0.0f, 1.0f };

  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0, 0.0, 1.0 };

  std::vector<glm::vec2> newPoints;
  std::vector<glm::vec4> newPointVelocities; // { x, y, dx, dy } normalized

  glm::vec2 currentVelocityNorm { 0.0f, 0.0f };
  
  AddRadialImpulseShader addRadialImpulseShader;
};


} // ofxMarkSynth
