//
//  ParticleSetMod.hpp
//  example_particles
//
//  Created by Steve Meyfroidt on 09/05/2025.
//

#pragma once

#include "ofxGui.h"
#include "Mod.hpp"
#include "ofxParticleSet.h"
#include "PingPongFbo.h"
#include "MultiplyColorShader.h"


namespace ofxMarkSynth {


class ParticleSetMod : public Mod {

public:
  ParticleSetMod(const std::string& name, const ModConfig&& config, const glm::vec2 fboSize);
  void update() override;
  void draw() override;
  void receive(int sinkId, const float& value) override;
  void receive(int sinkId, const glm::vec2& point) override;
  void receive(int sinkId, const glm::vec4& v) override;

  static constexpr int SINK_POINTS = 1;
  static constexpr int SINK_POINT_VELOCITIES = 2;
  static constexpr int SINK_SPIN = 10;
  static constexpr int SINK_COLOR = 20;

protected:
  void initParameters() override;

private:
  ofParameter<float> spinParameter { "Spin", 0.01, 0.0, 0.05 };
  ofParameter<ofFloatColor> colorParameter { "Color", ofColor::darkRed, ofColor(0, 255), ofColor(255, 255) };
  ofParameter<ofFloatColor> fadeParameter { "Fade", ofFloatColor { 1.0, 1.0, 1.0, 0.995 }, ofFloatColor { 0.9, 0.9, 0.9, 0.9}, ofFloatColor { 1.0, 1.0, 1.0, 1.0 } };

  std::vector<glm::vec4> newPoints; // { x, y, dx, dy }
  ParticleSet particleSet;
  
  PingPongFbo fbo;
  MultiplyColorShader fadeShader;
};


} // ofxMarkSynth
