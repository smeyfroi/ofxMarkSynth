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


namespace ofxMarkSynth {


class ParticleSetMod : public Mod {

public:
  ParticleSetMod(const std::string& name, const ModConfig&& config);
  void update() override;
  void receive(int sinkId, const float& value) override;
  void receive(int sinkId, const glm::vec2& point) override;
  void receive(int sinkId, const glm::vec4& v) override;

  static constexpr int SINK_POINTS = 1;
  static constexpr int SINK_POINT_VELOCITIES = 2;
  static constexpr int SINK_SPIN = 10;
  static constexpr int SINK_COLOR = 20;
  
  static constexpr int BLEND_STRATEGY_ADD = 0;
  static constexpr int BLEND_STRATEGY_ALPHA = 1;

protected:
  void initParameters() override;

private:
  ofParameter<float> spinParameter { "Spin", 0.03, 0.0, 0.05 };
  ofParameter<ofFloatColor> colorParameter { "Color", ofFloatColor(1.0, 1.0, 1.0, 1.0), ofFloatColor(0.0, 0.0, 0.0, 0.0), ofFloatColor(1.0, 1.0, 1.0, 1.0) };
  ofParameter<int> blendStrategy { "BlendStrategy", 0, 0, 1 };

  std::vector<glm::vec4> newPoints; // { x, y, dx, dy }
  ParticleSet particleSet;
};


} // ofxMarkSynth
