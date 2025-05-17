//
//  DrawPointsMod.hpp
//  example_points
//
//  Created by Steve Meyfroidt on 05/05/2025.
//

#pragma once

#include "ofxGui.h"
#include "Mod.hpp"


namespace ofxMarkSynth {


// TODO: Add an option to blur instead of fade
class DrawPointsMod : public Mod {

public:
  DrawPointsMod(const std::string& name, const ModConfig&& config);
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
  ofParameter<float> pointRadiusParameter { "PointRadius", 0.001, 0.0, 0.1 };
  ofParameter<ofFloatColor> colorParameter { "Color", ofColor::darkRed, ofColor(0, 255), ofColor(255, 255) };

  std::vector<glm::vec2> newPoints;
};


} // ofxMarkSynth
