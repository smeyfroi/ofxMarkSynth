//
//  SandLineMod.hpp
//  example_sandlines
//
//  Created by Steve Meyfroidt on 10/05/2025.
//

#pragma once

#include "ofxGui.h"
#include "Mod.hpp"
#include "PingPongFbo.h"


namespace ofxMarkSynth {


class SandLineMod : public Mod {

public:
  SandLineMod(const std::string& name, const ModConfig&& config);
  void update() override;
  void receive(int sinkId, const float& value) override;
  void receive(int sinkId, const glm::vec2& point) override;
  void receive(int sinkId, const glm::vec4& v) override;

  static constexpr int SINK_POINTS = 1;
  static constexpr int SINK_POINT_RADIUS = 10;
  static constexpr int SINK_POINT_COLOR = 20;

protected:
  void initParameters() override;

private:
  void drawSandLine(glm::vec2 p1, glm::vec2 p2, float drawScale);

  ofParameter<float> densityParameter { "Density", 0.05, 0.0, 0.5 };
  ofParameter<float> pointRadiusParameter { "PointRadius", 1.0, 0.0, 32.0 };
  ofParameter<ofFloatColor> colorParameter { "Color", ofFloatColor { 1.0, 1.0, 1.0, 1.0 }, ofFloatColor { 0.0, 0.0, 0.0, 0.0 }, ofFloatColor { 1.0, 1.0, 1.0, 1.0 } };
  ofParameter<float> alphaMultiplierParameter { "AlphaMultiplier", 0.05, 0.0, 1.0 };
  ofParameter<float> stdDevAlongParameter { "StdDevAlong", 0.5, 0.0, 1.0 };
  ofParameter<float> stdDevPerpendicularParameter { "StdDevPerpendicular", 0.01, 0.0, 0.1 };

  std::vector<glm::vec2> newPoints;
};


} // ofxMarkSynth
