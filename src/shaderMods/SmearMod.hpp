//
//  SmearMod.hpp
//  fingerprint-ipad-1
//
//  Created by Steve Meyfroidt on 01/08/2025.
//

#pragma once

#include "ofxGui.h"
#include "Mod.hpp"
#include "PingPongFbo.h"
#include "SmearShader.h"


namespace ofxMarkSynth {


class SmearMod : public Mod {

public:
  SmearMod(const std::string& name, const ModConfig&& config);
  void update() override;
  void receive(int sinkId, const glm::vec2& v) override;
  void receive(int sinkId, const float& v) override;

  static constexpr int SINK_VEC2 = 10;
  static constexpr int SINK_FLOAT = 11;

protected:
  void initParameters() override;

private:
  ofParameter<float> mixNewParameter { "MixNew", 0.9, 0.0, 1.0 };
  ofParameter<float> alphaMultiplierParameter { "AlphaMultiplier", 0.995, 0.9, 1.0 };
  ofParameter<glm::vec2> translateByParameter { "Translation", glm::vec2 { 0.0, 0.001 }, glm::vec2 { -0.01, -0.01 }, glm::vec2 { 0.01, 0.01 } };

  SmearShader smearShader;
};


} // ofxMarkSynth
