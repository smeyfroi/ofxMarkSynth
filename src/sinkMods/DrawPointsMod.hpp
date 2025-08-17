//
//  DrawPointsMod.hpp
//  example_points
//
//  Created by Steve Meyfroidt on 05/05/2025.
//

#pragma once

#include "Mod.hpp"


namespace ofxMarkSynth {


class DrawPointsMod : public Mod {

public:
  DrawPointsMod(const std::string& name, const ModConfig&& config);
  void update() override;
  void receive(int sinkId, const float& value) override;
  void receive(int sinkId, const glm::vec2& point) override;
  void receive(int sinkId, const glm::vec4& v) override;

  static constexpr int SINK_POINTS = 1;
  static constexpr int SINK_POINT_RADIUS = 10;
  static constexpr int SINK_POINT_RADIUS_VARIANCE = 11;
  static constexpr int SINK_POINT_RADIUS_VARIANCE_SCALE = 12;
  static constexpr int SINK_POINT_COLOR = 20;
  static constexpr int SINK_POINT_COLOR_MULTIPLIER = 21;

protected:
  void initParameters() override;

private:
  ofParameter<float> radiusParameter { "Radius", 0.001, 0.0, 0.1 };
  ofParameter<float> radiusVarianceParameter { "RadiusVariance", 0.0, 0.0, 1.0 };
  ofParameter<float> radiusVarianceScaleParameter { "RadiusVarianceScale", 0.001, 0.0, 1.0 };
  ofParameter<ofFloatColor> colorParameter { "Color", ofColor::darkRed, ofColor(0, 255), ofColor(255, 255) };
  ofParameter<float> colorMultiplierParameter { "ColorMultiplier", 1.0, 0.0, 1.0 };

  std::vector<glm::vec2> newPoints;
};


} // ofxMarkSynth
