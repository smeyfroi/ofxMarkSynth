//
//  TextMod.hpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 18/11/2025.
//

#pragma once

#include "Mod.hpp"
#include "ParamController.h"
#include "ofTrueTypeFont.h"
#include <filesystem>



namespace ofxMarkSynth {



class TextMod : public Mod {

public:
  TextMod(Synth* synthPtr, const std::string& name, ModConfig config, 
          const std::filesystem::path& fontPath);
  
  void update() override;
  void receive(int sinkId, const std::string& text) override;
  void receive(int sinkId, const float& value) override;
  void receive(int sinkId, const glm::vec2& point) override;
  void receive(int sinkId, const glm::vec4& v) override;
  void applyIntent(const Intent& intent, float strength) override;
  float getAgency() const override;
  
  static constexpr int SINK_TEXT = 1;
  static constexpr int SINK_POSITION = 10;
  static constexpr int SINK_FONT_SIZE = 20;
  static constexpr int SINK_COLOR = 30;
  static constexpr int SINK_ALPHA = 31;
  
protected:
  void initParameters() override;
  
private:
  // Font rendering
  ofTrueTypeFont font;
  std::filesystem::path fontPath;
  bool fontLoaded { false };
  int currentFontSize { 0 };
  
  // Parameters
  ofParameter<glm::vec2> positionParameter { "Position", {0.5, 0.5}, {0.0, 0.0}, {1.0, 1.0} };
  ParamController<glm::vec2> positionController { positionParameter };
  
  ofParameter<float> fontSizeParameter { "Font Size", 0.05, 0.01, 0.5 };
  ParamController<float> fontSizeController { fontSizeParameter };
  
  ofParameter<ofFloatColor> colorParameter { "Color", {1.0, 1.0, 1.0, 1.0}, {0.0, 0.0, 0.0, 0.0}, {1.0, 1.0, 1.0, 1.0} };
  ParamController<ofFloatColor> colorController { colorParameter };
  
  ofParameter<float> alphaParameter { "Alpha", 1.0, 0.0, 1.0 };
  ParamController<float> alphaController { alphaParameter };
  
  ofParameter<float> agencyFactorParameter { "Agency Factor", 1.0, 0.0, 1.0 };
  
  // Helpers
  void loadFontAtSize(int pixelSize);
  void renderText(const std::string& text);
};



} // ofxMarkSynth
