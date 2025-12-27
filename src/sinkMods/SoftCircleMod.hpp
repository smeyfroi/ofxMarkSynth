//
//  SoftCircleMod.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 25/05/2025.
//

#pragma once

#include <vector>
#include "core/Mod.hpp"
#include "core/ParamController.h"
#include "SoftCircleShader.h"


namespace ofxMarkSynth {



class SoftCircleMod : public Mod {

public:
  SoftCircleMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config);
  float getAgency() const override;
  void update() override;
  void receive(int sinkId, const float& value) override;
  void receive(int sinkId, const glm::vec2& point) override;
  void receive(int sinkId, const glm::vec4& v) override;
  void applyIntent(const Intent& intent, float strength) override;

  static constexpr int SINK_POINTS = 1;
  static constexpr int SINK_RADIUS = 10;
  static constexpr int SINK_COLOR = 20;
  static constexpr int SINK_COLOR_MULTIPLIER = 21;
  static constexpr int SINK_ALPHA_MULTIPLIER = 22;
  static constexpr int SINK_SOFTNESS = 30;

protected:
  void initParameters() override;

private:
  ofParameter<float> radiusParameter { "Radius", 0.005, 0.0, 0.25 };
  ParamController<float> radiusController { radiusParameter };
  ofParameter<ofFloatColor> colorParameter { "Colour", ofFloatColor { 0.5f, 0.5f, 0.5f, 0.5f }, ofFloatColor { 0.0f, 0.0f, 0.0f, 0.0f }, ofFloatColor { 1.0f, 1.0f, 1.0f, 1.0f } };
  ParamController<ofFloatColor> colorController { colorParameter };
  ofParameter<float> colorMultiplierParameter { "ColourMultiplier", 0.5, 0.0, 1.0 }; // RGB
  ParamController<float> colorMultiplierController { colorMultiplierParameter };
  ofParameter<float> alphaMultiplierParameter { "AlphaMultiplier", 0.2, 0.0, 1.0 }; // A
  ParamController<float> alphaMultiplierController { alphaMultiplierParameter };
  ofParameter<float> softnessParameter { "Softness", 0.3, 0.0, 1.0 };
  ParamController<float> softnessController { softnessParameter };
  ofParameter<int> falloffParameter { "Falloff", 0, 0, 1 }; // 0 = Glow, 1 = Dab
  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0, 0.0, 1.0 }; // 0.0 -> No agency; 1.0 -> Global synth agency

  std::vector<glm::vec2> newPoints;
  SoftCircleShader softCircleShader;
};



} // ofxMarkSynth
