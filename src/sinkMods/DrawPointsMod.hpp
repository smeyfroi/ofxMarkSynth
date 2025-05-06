//
//  DrawPointsMod.hpp
//  example_points
//
//  Created by Steve Meyfroidt on 05/05/2025.
//

#pragma once

#include "ofxGui.h"
#include "Mod.hpp"
#include "PingPongFbo.h"
#include "TranslateShader.h"
#include "MultiplyColorShader.h"


namespace ofxMarkSynth {


// TODO: Add an option to blur instead of fade
class DrawPointsMod : public Mod {

public:
  DrawPointsMod(const std::string& name, const ModConfig&& config, const glm::vec2 fboSize);
  void update() override;
  void draw() override;
  void receive(int sinkId, const float& value) override;
  void receive(int sinkId, const glm::vec2& point) override;
  void receive(int sinkId, const glm::vec4& v) override;

  static constexpr int SINK_POINTS = 1;
  static constexpr int SINK_POINT_RADIUS = 10;
  static constexpr int SINK_POINT_COLOR = 20;

protected:
  void initParameters() override;

private:
  ofParameter<float> pointRadiusParameter { "PointRadius", 2.0, 0.0, 32.0 };
  ofParameter<ofFloatColor> colorParameter { "Color", ofColor::darkRed, ofColor(0, 255), ofColor(255, 255) };
  ofParameter<ofFloatColor> fadeParameter { "Fade", ofFloatColor { 1.0, 1.0, 1.0, 0.995 }, ofFloatColor { 0.9, 0.9, 0.9, 0.9}, ofFloatColor { 1.0, 1.0, 1.0, 1.0 } };
  ofParameter<glm::vec2> translationParameter { "Translation", glm::vec2 { 0.0, 0.001 }, glm::vec2 { 0.0, 0.0 }, glm::vec2 { 0.01, 0.01 } };

  std::vector<glm::vec2> newPoints;
  
  PingPongFbo fbo;
  MultiplyColorShader fadeShader;
  TranslateShader translateShader;
};


} // ofxMarkSynth
