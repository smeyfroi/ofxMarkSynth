//
//  SoftCircleMod.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 25/05/2025.
//

#pragma once

#include "Mod.hpp"
#include "SoftCircleShader.h"


namespace ofxMarkSynth {


class SoftCircleMod : public Mod {

public:
  SoftCircleMod(const std::string& name, const ModConfig&& config);
  void update() override;
  void receive(int sinkId, const float& value) override;
  void receive(int sinkId, const glm::vec2& point) override;
  void receive(int sinkId, const glm::vec4& v) override;

  static constexpr int SINK_POINTS = 1;
  static constexpr int SINK_POINT_RADIUS = 10;
  static constexpr int SINK_POINT_COLOR = 20;
  static constexpr int SINK_POINT_COLOR_MULTIPLIER = 21;
  static constexpr int SINK_POINT_SOFTNESS = 30;

protected:
  void initParameters() override;

private:
  ofParameter<float> radiusParameter { "Radius", 0.01, 0.0, 0.25 };
  ofParameter<ofFloatColor> colorParameter { "Color", ofColor::darkRed, ofColor(0, 255), ofColor(255, 255) };
  ofParameter<float> colorMultiplierParameter { "ColorMultiplier", 0.01, 0.0, 1.0 }; // RGB
  ofParameter<float> alphaMultiplierParameter { "AlphaMultiplier", 0.2, 0.0, 1.0 }; // A
  ofParameter<float> softnessParameter { "Softness", 0.3, 0.0, 1.0 };

  std::vector<glm::vec2> newPoints;
  SoftCircleShader softCircleShader;
};


} // ofxMarkSynth
