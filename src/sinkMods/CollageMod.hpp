//
//  CollageMod.hpp
//  example_collage
//
//  Created by Steve Meyfroidt on 21/05/2025.
//

#include "ofxGui.h"
#include "Mod.hpp"
#include "PingPongFbo.h"
#include "MaskShader.h"
#include "AddTextureThresholded.h"
#include "ParamController.h"



namespace ofxMarkSynth {



class CollageMod : public Mod {

public:
  CollageMod(Synth* synthPtr, const std::string& name, const ModConfig&& config);
  void update() override;
  void receive(int sinkId, const ofPath& path) override;
  void receive(int sinkId, const ofFbo& value) override;
  void receive(int sinkId, const glm::vec4& v) override;
  void applyIntent(const Intent& intent, float strength) override;

  static constexpr std::string OUTLINE_LAYERPTR_NAME { "outlines" };

  static constexpr int SINK_PATH = 1;
  static constexpr int SINK_SNAPSHOT_FBO = 11;
  static constexpr int SINK_COLOR = 20;

protected:
  void initParameters() override;

  ofPath path;
  ofFbo snapshotFbo;

  ofParameter<ofFloatColor> colorParameter { "Color", ofFloatColor { 1.0, 1.0, 1.0, 1.0 }, ofFloatColor { 0.0, 0.0, 0.0, 0.0 }, ofFloatColor { 1.0, 1.0, 1.0, 1.0 } };
  ParamController<ofFloatColor> colorController { colorParameter };
  ofParameter<float> saturationParameter { "Saturation", 1.5, 0.0, 4.0 };
  ParamController<float> saturationController { saturationParameter };
  ofParameter<float> outlineParameter { "Outline", 1.0f, 0.0f, 1.0f }; // float for lerp between 0.0 and 1.0
  ParamController<float> outlineController { outlineParameter };
  ofParameter<int> strategyParameter { "Strategy", 1, 0, 2 }; // 0=tint; 1=add tinted pixels; 2=add pixels
};



} // ofxMarkSynth
