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
  void receive(int sinkId, const float& value) override;
  void receive(int sinkId, const ofFbo& value) override;

  static constexpr int SINK_VEC2 = 10;
  static constexpr int SINK_FLOAT = 11;
  static constexpr int SINK_FIELD_1_FBO = 20;
  static constexpr int SINK_FIELD_2_FBO = 21;

protected:
  void initParameters() override;

private:
  ofParameter<float> mixNewParameter { "MixNew", 0.9, 0.0, 1.0 };
  ofParameter<float> alphaMultiplierParameter { "AlphaMultiplier", 0.998, 0.9, 1.0 };
//  ofParameter<glm::vec2> translateByParameter { "Translation", glm::vec2 { 0.0, 0.001 }, glm::vec2 { -0.01, -0.01 }, glm::vec2 { 0.01, 0.01 } };
  ofParameter<glm::vec2> translateByParameter { "Translation", glm::vec2 { 0.0, 0.0 }, glm::vec2 { -0.01, -0.01 }, glm::vec2 { 0.01, 0.01 } };
  ofParameter<float> field1MultiplierParameter { "Field1Multiplier", 0.001, 0.0, 0.1 };
//  ofParameter<glm::vec2> field1BiasParameter { "Field1Bias", glm::vec2 { -0.5, -0.5 }, glm::vec2 { -1.0, -1.0 }, glm::vec2 { 1.0, 1.0 } };
  ofParameter<glm::vec2> field1BiasParameter { "Field1Bias", glm::vec2 { 0.0, 0.0 }, glm::vec2 { -1.0, -1.0 }, glm::vec2 { 1.0, 1.0 } };
  ofParameter<float> field2MultiplierParameter { "Field2Multiplier", 0.005, 0.0, 0.1 };
//  ofParameter<glm::vec2> field2BiasParameter { "Field2Bias", glm::vec2 { -0.5, -0.5 }, glm::vec2 { -1.0, -1.0 }, glm::vec2 { 1.0, 1.0 } };
  ofParameter<glm::vec2> field2BiasParameter { "Field2Bias", glm::vec2 { 0.0, 0.0 }, glm::vec2 { -1.0, -1.0 }, glm::vec2 { 1.0, 1.0 } };

  SmearShader smearShader;
  
  ofFbo field1Fbo, field2Fbo;
  bool useField2;
};


} // ofxMarkSynth
