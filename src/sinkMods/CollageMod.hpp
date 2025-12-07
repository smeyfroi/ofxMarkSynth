//
//  CollageMod.hpp
//  example_collage
//
//  Created by Steve Meyfroidt on 21/05/2025.
//

#pragma once

#include "ofxGui.h"
#include "Mod.hpp"
#include "PingPongFbo.h"
#include "ParamController.h"



namespace ofxMarkSynth {



class CollageMod : public Mod {

public:
  CollageMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config);
  float getAgency() const override;
  void update() override;
  void receive(int sinkId, const ofPath& path) override;
  void receive(int sinkId, const ofTexture& texture) override;
  void receive(int sinkId, const glm::vec4& v) override;
  void applyIntent(const Intent& intent, float strength) override;

  static constexpr std::string OUTLINE_LAYERPTR_NAME { "outlines" };

  static constexpr int SINK_PATH = 1;
  static constexpr int SINK_SNAPSHOT_TEXTURE = 11;
  static constexpr int SINK_COLOR = 20;

protected:
  void initParameters() override;

  ofPath path;
  ofTexture snapshotTexture;

  ofParameter<ofFloatColor> colorParameter { "Colour", ofFloatColor { 1.0, 1.0, 1.0, 1.0 }, ofFloatColor { 0.0, 0.0, 0.0, 0.0 }, ofFloatColor { 1.0, 1.0, 1.0, 1.0 } };
  ParamController<ofFloatColor> colorController { colorParameter };
  ofParameter<float> saturationParameter { "Saturation", 1.5, 0.0, 4.0 };
  ParamController<float> saturationController { saturationParameter };
  ofParameter<float> outlineParameter { "Outline", 1.0f, 0.0f, 1.0f }; // float for lerp between 0.0 and 1.0
  ParamController<float> outlineController { outlineParameter };
  ofParameter<int> strategyParameter { "Strategy", 1, 0, 2 }; // 0=tint; 1=add tinted pixels; 2=add pixels
  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0, 0.0, 1.0 };
};



} // ofxMarkSynth
