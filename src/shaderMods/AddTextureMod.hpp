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
#include "IntentParamController.h"


namespace ofxMarkSynth {


class AddTextureMod : public Mod {

public:
  AddTextureMod(Synth* synthPtr, const std::string& name, const ModConfig&& config);
  void update() override;
  void receive(int sinkId, const float& v) override;
  void receive(int sinkId, const ofFloatPixels& pixels) override;
  void applyIntent(const Intent& intent, float strength) override;

  static constexpr int SINK_SCALE = 10;
//  static constexpr int SINK_TARGET_FBO = SINK_FBOPTR_BEGIN;
  static constexpr int SINK_ADD_PIXELS = 100;
  
protected:
  void initParameters() override;

private:
  ofParameter<float> scaleParameter { "Scale", 0.05, 0.0, 1.0 };
  IntentParamController<float> scaleController { scaleParameter };

  ofTexture addTexture;
  AddTextureShader addTextureShader;
};


} // ofxMarkSynth
