//
//  SoftCircleMod.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 25/05/2025.
//

#pragma once

#include <optional>
#include <vector>

#include "SoftCircleShader.h"
#include "core/ColorRegister.hpp"
#include "core/Mod.hpp"
#include "core/ParamController.h"

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
  static constexpr int SINK_POINT_VELOCITY = 2;
  static constexpr int SINK_RADIUS = 10;
  static constexpr int SINK_COLOR = 20;
  static constexpr int SINK_COLOR_MULTIPLIER = 21;
  static constexpr int SINK_ALPHA_MULTIPLIER = 22;
  static constexpr int SINK_SOFTNESS = 30;
  static constexpr int SINK_EDGE_MOD = 40;
  static constexpr int SINK_CHANGE_KEY_COLOUR = 90;

protected:
  void initParameters() override;

private:
  ofParameter<float> radiusParameter { "Radius", 0.005, 0.0, 0.1 };
  ParamController<float> radiusController { radiusParameter };
  ofParameter<ofFloatColor> colorParameter { "Colour",
                                            ofFloatColor { 0.5f, 0.5f, 0.5f, 0.5f },
                                            ofFloatColor { 0.0f, 0.0f, 0.0f, 0.0f },
                                            ofFloatColor { 1.0f, 1.0f, 1.0f, 1.0f } };
  ParamController<ofFloatColor> colorController { colorParameter };

  // Key colour register: pipe-separated vec4 list. Example:
  // "0,0,0,0.5 | 1,1,1,0.5"
  ofParameter<std::string> keyColoursParameter { "KeyColours", "" };
  ColorRegister keyColourRegister;
  bool keyColourRegisterInitialized { false };

  ofParameter<float> colorMultiplierParameter { "ColourMultiplier", 0.5, 0.0, 1.0 }; // RGB
  ParamController<float> colorMultiplierController { colorMultiplierParameter };
  ofParameter<float> alphaMultiplierParameter { "AlphaMultiplier", 0.2, 0.0, 1.0 }; // A
  ParamController<float> alphaMultiplierController { alphaMultiplierParameter };

  // Alpha scale in log2 space (<= 0). Helps keep AlphaMultiplier in a usable range.
  ofParameter<float> alphaPreScaleExpParameter { "AlphaPreScaleExp", 0.0f, -12.0f, 0.0f };

  ofParameter<float> softnessParameter { "Softness", 0.3, 0.0, 1.0 };
  ParamController<float> softnessController { softnessParameter };
  ofParameter<int> falloffParameter { "Falloff", 0, 0, 1 }; // 0 = Glow, 1 = Dab
  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0, 0.0, 1.0 }; // 0.0 -> No agency; 1.0 -> Global synth agency

  // Organic brush (velocity/curvature/edge modulation)
  ofParameter<int> useVelocityParameter { "UseVelocity", 0, 0, 1 };
  ofParameter<float> velocitySpeedMinParameter { "VelocitySpeedMin", 0.0f, 0.0f, 1.0f };
  ofParameter<float> velocitySpeedMaxParameter { "VelocitySpeedMax", 0.05f, 0.0f, 1.0f };
  ofParameter<float> velocityStretchParameter { "VelocityStretch", 0.0f, 0.0f, 4.0f };
  ofParameter<float> velocityAngleInfluenceParameter { "VelocityAngleInfluence", 1.0f, 0.0f, 1.0f };
  ofParameter<int> preserveAreaParameter { "PreserveArea", 1, 0, 1 };
  ofParameter<float> maxAspectParameter { "MaxAspect", 4.0f, 1.0f, 16.0f };

  ofParameter<float> curvatureAlphaBoostParameter { "CurvatureAlphaBoost", 0.0f, 0.0f, 2.0f };
  ofParameter<float> curvatureRadiusBoostParameter { "CurvatureRadiusBoost", 0.0f, 0.0f, 2.0f };
  ofParameter<float> curvatureSmoothingParameter { "CurvatureSmoothing", 0.9f, 0.0f, 0.999f };

  // Edge modulation amount is:
  //   EdgeAmount + EdgeAmountFromDriver * EdgeModSink
  ofParameter<float> edgeAmountParameter { "EdgeAmount", 0.0f, 0.0f, 1.0f };
  ofParameter<float> edgeAmountFromDriverParameter { "EdgeAmountFromDriver", 0.0f, 0.0f, 1.0f };
  ofParameter<float> edgeSharpnessParameter { "EdgeSharpness", 1.0f, 0.1f, 4.0f };
  ofParameter<float> edgeFreq1Parameter { "EdgeFreq1", 3.0f, 0.0f, 64.0f };
  ofParameter<float> edgeFreq2Parameter { "EdgeFreq2", 5.0f, 0.0f, 64.0f };
  ofParameter<float> edgeFreq3Parameter { "EdgeFreq3", 9.0f, 0.0f, 64.0f };
  ofParameter<int> edgePhaseFromVelocityParameter { "EdgePhaseFromVelocity", 1, 0, 1 };

  struct Stamp {
    glm::vec2 pointNorm { 0.0f, 0.0f };
    glm::vec2 velocityNorm { 0.0f, 0.0f };
    bool hasVelocity { false };
    float edgeDriver { 0.0f };
    float curvature { 0.0f };
  };

  std::vector<Stamp> newStamps;

  float currentEdgeDriver { 0.0f };
  std::optional<glm::vec2> lastPointOpt;
  std::optional<glm::vec2> lastDirectionOpt;
  float lastCurvature { 0.0f };

  SoftCircleShader softCircleShader;
};

} // namespace ofxMarkSynth
