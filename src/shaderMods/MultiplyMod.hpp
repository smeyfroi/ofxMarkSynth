//
//  MultiplyMod.hpp
//  example_fade
//
//  Created by Steve Meyfroidt on 11/05/2025.
//

#pragma once

#include "ofxGui.h"
#include "Mod.hpp"
#include "PingPongFbo.h"
#include "MultiplyColorShader.h"


namespace ofxMarkSynth {


class MultiplyMod : public Mod {

public:
  MultiplyMod(const std::string& name, const ModConfig&& config);
  void update() override;
  void receive(int sinkId, const float& v) override;

  static constexpr int SINK_FADE = 10;

protected:
  void initParameters() override;

private:
  ofParameter<float> multiplyByParameter { "Multiply By", 0.995, 0.0, 1.0 };
};


} // ofxMarkSynth
