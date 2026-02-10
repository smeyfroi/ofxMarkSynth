//  CollageMod.hpp
//  example_collage
//
//  Created by Steve Meyfroidt on 21/05/2025.
//

#pragma once

#include "PingPongFbo.h"
#include "core/ColorRegister.hpp"
#include "core/Mod.hpp"
#include "core/ParamController.h"

namespace ofxMarkSynth {

class CollageMod : public Mod {

public:
  CollageMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config);
  float getAgency() const override;
  void update() override;
  void receive(int sinkId, const ofPath& path) override;
  void receive(int sinkId, const ofTexture& texture) override;
  void receive(int sinkId, const glm::vec4& v) override;
  void receive(int sinkId, const float& value) override;
  void applyIntent(const Intent& intent, float strength) override;

  static constexpr std::string OUTLINE_LAYERPTR_NAME { "outlines" };

  static constexpr int SINK_PATH = 1;
  static constexpr int SINK_SNAPSHOT_TEXTURE = 11;
  static constexpr int SINK_COLOR = 20;
  static constexpr int SINK_OUTLINE_COLOR = 21;
  static constexpr int SINK_CHANGE_KEY_COLOUR = 90;

protected:
  void initParameters() override;

private:
  void drawOutline(std::shared_ptr<PingPongFbo> fboPtr, float outlineAlphaFactor);
  void drawStrategyTintFill(const ofFloatColor& tintColor);
  void drawStrategySnapshot(const ofFloatColor& tintColor);

protected:
  ofPath path;
  ofTexture snapshotTexture;

  ofParameter<ofFloatColor> colorParameter { "Colour",
                                            ofFloatColor { 1.0, 1.0, 1.0, 1.0 },
                                            ofFloatColor { 0.0, 0.0, 0.0, 0.0 },
                                            ofFloatColor { 1.0, 1.0, 1.0, 1.0 } };
  ParamController<ofFloatColor> colorController { colorParameter };

  // Key colour register: pipe-separated vec4 list. Example:
  // "0,0,0,1 | 0.5,0.5,0.5,1 | 1,1,1,1"
  ofParameter<std::string> keyColoursParameter { "KeyColours", "" };
  ColorRegister keyColourRegister;
  bool keyColourRegisterInitialized { false };

  ofParameter<float> saturationParameter { "Saturation", 1.5, 0.0, 4.0 };
  ParamController<float> saturationController { saturationParameter };
  ofParameter<float> outlineAlphaFactorParameter { "OutlineAlphaFactor", 1.0f, 0.0f, 1.0f };
  ParamController<float> outlineAlphaFactorController { outlineAlphaFactorParameter };
  ofParameter<float> outlineWidthParameter { "OutlineWidth", 12.0f, 0.0f, 50.0f }; // pixels
  ParamController<float> outlineWidthController { outlineWidthParameter };
  ofParameter<ofFloatColor> outlineColorParameter { "OutlineColour",
                                                    ofFloatColor { 1.0, 1.0, 1.0, 1.0 },
                                                    ofFloatColor { 0.0, 0.0, 0.0, 0.0 },
                                                    ofFloatColor { 1.0, 1.0, 1.0, 1.0 } };
  ParamController<ofFloatColor> outlineColorController { outlineColorParameter };
  ofParameter<int> strategyParameter { "Strategy", 1, 0, 2 }; // 0=tint; 1=add tinted pixels; 2=add pixels
  ofParameter<int> blendModeParameter { "BlendMode", 1, 0, 4 }; // 0=ALPHA, 1=SCREEN, 2=ADD, 3=MULTIPLY, 4=SUBTRACT
  ofParameter<float> opacityParameter { "Opacity", 1.0f, 0.0f, 1.0f };
  ParamController<float> opacityController { opacityParameter };
  ofParameter<float> minDrawIntervalParameter { "MinDrawInterval", 0.0f, 0.0f, 1.0f }; // seconds
  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0, 0.0, 1.0 };

  float lastDrawTime { 0.0f };
};

} // ofxMarkSynth
