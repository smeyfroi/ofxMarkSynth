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


namespace ofxMarkSynth {


class CollageMod : public Mod {

public:
  CollageMod(const std::string& name, const ModConfig&& config);
  void update() override;
  void receive(int sinkId, const ofPath& path) override;
  void receive(int sinkId, const ofPixels& pixels) override;
  void receive(int sinkId, const glm::vec4& v) override;
  
  static constexpr int SINK_PATH = 1;
  static constexpr int SINK_PIXELS = 10;
  static constexpr int SINK_COLOR = 20;

protected:
  void initParameters() override;

private:
  void drawCollagePiece(const ofPath& path, const ofPixels& pixels, float drawScale);

  ofPath path;
  ofTexture collageSourceTexture;
  MaskShader maskShader;

  ofParameter<ofFloatColor> colorParameter { "Color", ofFloatColor { 1.0, 1.0, 1.0, 1.0 }, ofFloatColor { 0.0, 0.0, 0.0, 0.0 }, ofFloatColor { 1.0, 1.0, 1.0, 1.0 } };
  ofParameter<float> strengthParameter { "Strength", 0.1, 0.0, 2.0 };
};


} // ofxMarkSynth
