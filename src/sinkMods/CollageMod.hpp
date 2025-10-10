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


namespace ofxMarkSynth {


class CollageMod : public Mod {

public:
  CollageMod(const std::string& name, const ModConfig&& config);
  void update() override;
  void receive(int sinkId, const ofPath& path) override;
  void receive(int sinkId, const ofFbo& value) override;
  void receive(int sinkId, const glm::vec4& v) override;
  
  static constexpr std::string OUTLINE_FBOPTR_NAME { "outlines" };

  static constexpr int SINK_PATH = 1;
  static constexpr int SINK_SNAPSHOT_FBO = 11;
  static constexpr int SINK_COLOR = 20;

protected:
  void initParameters() override;

  ofPath path;
  ofFbo snapshotFbo;

  ofParameter<ofFloatColor> colorParameter { "Color", ofFloatColor { 1.0, 1.0, 1.0, 1.0 }, ofFloatColor { 0.0, 0.0, 0.0, 0.0 }, ofFloatColor { 1.0, 1.0, 1.0, 1.0 } };
  ofParameter<float> strengthParameter { "Strength", 1.0, 0.0, 4.0 };
  ofParameter<int> strategyParameter { "Strategy", 1, 0, 2 }; // 0=tint; 1=add tinted pixels; 2=add pixels
  ofParameter<bool> outlineParameter { "Outline", true };
};


} // ofxMarkSynth
