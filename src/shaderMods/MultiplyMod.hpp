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
  void draw() override;
  void receive(int sinkId, const FboPtr& fboPtr) override;
  void receive(int sinkId, const glm::vec4& v) override;

  static constexpr int SINK_FBO = 1;
  static constexpr int SINK_VEC4 = 10;
  static constexpr int SOURCE_FBO = 2;

protected:
  void initParameters() override;

private:
  ofParameter<glm::vec4> multiplyByParameter { "Multiply By", glm::vec4 { 1.0, 1.0, 1.0, 0.995 }, glm::vec4 { 0.0, 0.0, 0.0, 0.0 }, glm::vec4 { 1.0, 1.0, 1.0, 1.0 } };

  MultiplyColorShader fadeShader;
  
  FboPtr fboPtr;
};


} // ofxMarkSynth
