//
//  AddTextureMod.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 30/05/2025.
//


#pragma once

#include "ofxGui.h"
#include "Mod.hpp"
#include "PingPongFbo.h"
#include "AddTextureShader.h"


namespace ofxMarkSynth {


class AddTextureMod : public Mod {

public:
  AddTextureMod(const std::string& name, const ModConfig&& config);
  void update() override;
  void receive(int sinkId, const float& v) override;
  void receive(int sinkId, const ofFloatPixels& pixels) override;

  static constexpr int SINK_SCALE = 10;
  static constexpr int SINK_TARGET_FBO = SINK_FBO_BEGIN;
  static constexpr int SINK_ADD_PIXELS = 100;
  
protected:
  void initParameters() override;

private:
  ofParameter<float> scaleParameter { "Scale", 0.05, 0.0, 1.0 };

  ofTexture addTexture;
  AddTextureShader addTextureShader;
};


} // ofxMarkSynth
