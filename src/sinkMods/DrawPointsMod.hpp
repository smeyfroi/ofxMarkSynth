//
//  DrawPointsMod.hpp
//  example_points
//
//  Created by Steve Meyfroidt on 05/05/2025.
//

#pragma once

#include "ofxGui.h"
#include "Mod.hpp"
#include "ofFbo.h"


namespace ofxMarkSynth {


class DrawPointsMod : public Mod {

public:
  DrawPointsMod(const std::string& name, const ModConfig&& config, const glm::vec2 fboSize);
  void update() override;
  void draw() override;
  void receive(int sinkId, const float& value) override;
  void receive(int sinkId, const glm::vec2& point) override;

  static constexpr int SINK_POINTS = 1;
  static constexpr int SINK_POINT_RADIUS = 10;

protected:
  void initParameters() override;

private:
  ofParameter<float> pointRadiusParameter { "PointRadius", 1.0, 0.0, 4.0 };
//  ofParameter<float> pointFadeParameter { "PointFade", 0.96, 0.9, 1.0 };
  ofParameter<ofFloatColor> colorParameter { "Color", ofColor::yellow, ofColor(0, 255), ofColor(255, 255) };

  std::vector<glm::vec2> newPoints;
  
  ofFbo fbo;
};


} // ofxMarkSynth
