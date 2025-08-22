//
//  FadeMod.hpp
//
//  Created by Steve Meyfroidt on 25/07/2025.
//

#pragma once

#include "ofxGui.h"
#include "Mod.hpp"
#include "TranslateEffect.h"
#include "FadeEffect.h"


namespace ofxMarkSynth {


class FadeMod : public Mod {

public:
  FadeMod(const std::string& name, const ModConfig&& config);
  void update() override;
  void receive(int sinkId, const float& v) override;
  void receive(int sinkId, const glm::vec2& v) override;

  static constexpr int SINK_ALPHA = 10;
  static constexpr int SINK_TRANSLATION = 11;

protected:
  void initParameters() override;

private:
  ofParameter<glm::vec2> translationParameter { "Translation", glm::vec2 { 0.0, 0.0 }, glm::vec2 { -0.01, -0.01 }, glm::vec2 { 0.01, 0.01 } };
  ofParameter<float> alphaParameter { "Translation Alpha", 1.0, 0.99, 1.0 };
  ofParameter<float> fadeAmountParameter { "Fade Amount", 0.0001, 0.0, 0.01 };

  TranslateEffect translateEffect;
  FadeEffect fadeEffect;
};


} // ofxMarkSynth
