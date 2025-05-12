//
//  TranslateMod.hpp
//  example_fade
//
//  Created by Steve Meyfroidt on 11/05/2025.
//

#pragma once

#include "ofxGui.h"
#include "Mod.hpp"
#include "PingPongFbo.h"
#include "TranslateShader.h"


namespace ofxMarkSynth {


class TranslateMod : public Mod {

public:
  TranslateMod(const std::string& name, const ModConfig&& config);
  void update() override;
  void receive(int sinkId, const glm::vec2& v) override;

  static constexpr int SINK_VEC2 = 10;

protected:
  void initParameters() override;

private:
  ofParameter<glm::vec2> translateByParameter { "Translate By", glm::vec2 { 0.0, 0.001 }, glm::vec2 { -0.01, -0.01 }, glm::vec2 { 0.01, 0.01 } };

  TranslateShader translateShader;
};


} // ofxMarkSynth
