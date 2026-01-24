//
//  TextMod.hpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 18/11/2025.
//

#pragma once

#include "core/ColorRegister.hpp"
#include "core/FontStash2Cache.hpp"
#include "core/Mod.hpp"
#include "core/ParamController.h"
#include <memory>
#include <vector>

namespace ofxMarkSynth {

class TextMod : public Mod {

public:
  TextMod(std::shared_ptr<Synth> synthPtr,
          const std::string& name,
          ModConfig config,
          std::shared_ptr<FontStash2Cache> fontCache);

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
  static constexpr int SINK_DRAW_DURATION_SEC = 40;
  static constexpr int SINK_ALPHA_FACTOR = 41;
  static constexpr int SINK_CHANGE_KEY_COLOUR = 90;

protected:
  void initParameters() override;

private:
  struct DrawEvent {
    std::string text;
    glm::vec2 positionNorm { 0.5f, 0.5f };
    ofFloatColor baseColor { 1.0f, 1.0f, 1.0f, 1.0f };
    int pixelSize { 0 };
    float startTimeSec { 0.0f };
    float durationSec { 1.0f };
    float alphaFactor { 0.2f };

    // For incremental fades on accumulating layers
    float applied { 0.0f };
  };

  // Font cache (shared, pre-loaded at startup)
  std::shared_ptr<FontStash2Cache> fontCachePtr;

  // State
  std::string currentText;
  std::vector<DrawEvent> drawEvents;

  // Parameters
  ofParameter<glm::vec2> positionParameter { "Position", { 0.5, 0.5 }, { 0.0, 0.0 }, { 1.0, 1.0 } };
  ParamController<glm::vec2> positionController { positionParameter };

  ofParameter<float> fontSizeParameter { "FontSize", 0.05, 0.01, 0.08 };
  ParamController<float> fontSizeController { fontSizeParameter };

  ofParameter<ofFloatColor> colorParameter { "Colour", { 1.0, 1.0, 1.0, 1.0 }, { 0.0, 0.0, 0.0, 0.0 }, { 1.0, 1.0, 1.0, 1.0 } };
  ParamController<ofFloatColor> colorController { colorParameter };

  // Key colour register: pipe-separated vec4 list. Example:
  // "0,0,0,1 | 1,1,1,1"
  ofParameter<std::string> keyColoursParameter { "KeyColours", "" };
  ColorRegister keyColourRegister;
  bool keyColourRegisterInitialized { false };

  ofParameter<float> alphaParameter { "Alpha", 1.0, 0.0, 1.0 };
  ParamController<float> alphaController { alphaParameter };

  ofParameter<float> drawDurationSecParameter { "DrawDurationSec", 1.0, 0.1, 10.0 };
  ParamController<float> drawDurationSecController { drawDurationSecParameter };

  ofParameter<float> alphaFactorParameter { "AlphaFactor", 0.2, 0.0, 1.0 };
  ParamController<float> alphaFactorController { alphaFactorParameter };

  ofParameter<int> maxDrawEventsParameter { "MaxDrawEvents", 8, 1, 64 };
  ofParameter<int> minFontPxParameter { "MinFontPx", 8, 1, 128 };

  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0, 0.0, 1.0 };

  // Helpers
  int resolvePixelSize(float normalizedFontSize, float fboHeight) const;
  void pushDrawEvent(const std::string& text);
  void drawEvent(DrawEvent& e, const DrawingLayerPtr& drawingLayerPtr);
};

} // ofxMarkSynth
