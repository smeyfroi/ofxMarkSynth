//
//  TextMod.cpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 18/11/2025.
//

#include "TextMod.hpp"
#include "IntentMapping.hpp"
#include "Synth.hpp"



namespace ofxMarkSynth {



TextMod::TextMod(Synth* synthPtr, const std::string& name, ModConfig config, 
                 const std::filesystem::path& fontPath_)
: Mod { synthPtr, name, std::move(config) },
  fontPath { fontPath_ }
{
  sinkNameIdMap = {
    { "Text", SINK_TEXT },
    { positionParameter.getName(), SINK_POSITION },
    { fontSizeParameter.getName(), SINK_FONT_SIZE },
    { colorParameter.getName(), SINK_COLOR },
    { alphaParameter.getName(), SINK_ALPHA }
  };
  
  sourceNameControllerPtrMap = {
    { positionParameter.getName(), &positionController },
    { fontSizeParameter.getName(), &fontSizeController },
    { colorParameter.getName(), &colorController },
    { alphaParameter.getName(), &alphaController }
  };
}

void TextMod::initParameters() {
  parameters.add(positionParameter);
  parameters.add(fontSizeParameter);
  parameters.add(colorParameter);
  parameters.add(alphaParameter);
  parameters.add(agencyFactorParameter);
}

float TextMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void TextMod::update() {
  syncControllerAgencies();
  // Update all parameter controllers
  positionController.update();
  fontSizeController.update();
  colorController.update();
  alphaController.update();
}

void TextMod::receive(int sinkId, const std::string& text) {
  switch (sinkId) {
    case SINK_TEXT:
      ofLogVerbose("TextMod") << "Received text: " << text;
      renderText(text);
      break;
    default:
      ofLogError("TextMod") << "String receive for unknown sinkId " << sinkId;
  }
}

void TextMod::receive(int sinkId, const float& value) {
  switch (sinkId) {
    case SINK_FONT_SIZE:
      fontSizeController.updateAuto(value, getAgency());
      break;
    case SINK_ALPHA:
      alphaController.updateAuto(value, getAgency());
      break;
    case SINK_CHANGE_LAYER:
      if (value > 0.5) {
        ofLogNotice("TextMod") << "SINK_CHANGE_LAYER: changing layer";
        changeDrawingLayer();
      }
      break;
    default:
      ofLogError("TextMod") << "Float receive for unknown sinkId " << sinkId;
  }
}

void TextMod::receive(int sinkId, const glm::vec2& point) {
  switch (sinkId) {
    case SINK_POSITION:
      positionController.updateAuto(point, getAgency());
      break;
    default:
      ofLogError("TextMod") << "glm::vec2 receive for unknown sinkId " << sinkId;
  }
}

void TextMod::receive(int sinkId, const glm::vec4& v) {
  switch (sinkId) {
    case SINK_COLOR:
      colorController.updateAuto(ofFloatColor { v.r, v.g, v.b, v.a }, getAgency());
      break;
    default:
      ofLogError("TextMod") << "glm::vec4 receive for unknown sinkId " << sinkId;
  }
}

void TextMod::applyIntent(const Intent& intent, float strength) {
  
  // Granularity → Font Size (scale of features)
  float fontSizeI = exponentialMap(intent.getGranularity(), fontSizeController);
  fontSizeController.updateIntent(fontSizeI, strength);
  
  // Energy → Color
  ofFloatColor colorI = energyToColor(intent);
  colorI.a = linearMap(intent.getDensity(), 0.5f, 1.0f);
  colorController.updateIntent(colorI, strength);
  
  // Density → Alpha
  float alphaI = linearMap(intent.getDensity(), alphaParameter);
  alphaController.updateIntent(alphaI, strength);
  
  // Chaos → Position jitter (randomize position around current value)
  float chaos = intent.getChaos();
  if (chaos > 0.1f) {
    glm::vec2 basePos = positionController.value;
    float jitterAmount = chaos * 0.15f; // max 15% of screen jitter at full chaos
    glm::vec2 jitteredPos = basePos + glm::vec2(
      ofRandom(-jitterAmount, jitterAmount),
      ofRandom(-jitterAmount, jitterAmount)
    );
    // Clamp to valid range
    jitteredPos.x = std::clamp(jitteredPos.x, 0.05f, 0.95f);
    jitteredPos.y = std::clamp(jitteredPos.y, 0.05f, 0.95f);
    positionController.updateIntent(jitteredPos, strength * chaos);
  }
}

void TextMod::loadFontAtSize(int pixelSize) {
  if (pixelSize == currentFontSize && fontLoaded) return;
  
  bool success = font.load(fontPath.string(), pixelSize, true, true);
  if (success) {
    fontLoaded = true;
    currentFontSize = pixelSize;
    ofLogNotice("TextMod") << "Loaded font at size " << pixelSize;
  } else {
    ofLogError("TextMod") << "Failed to load font from " << fontPath;
  }
}

void TextMod::renderText(const std::string& text) {
  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto fboPtr = drawingLayerPtrOpt.value()->fboPtr;
  
  // Calculate pixel size from normalized parameter
  int pixelSize = static_cast<int>(fontSizeController.value * fboPtr->getHeight());
  if (pixelSize < 8) pixelSize = 8; // Minimum readable size
  
  // Round to nearest 5 pixels to avoid excessive font reloads during smooth transitions
  pixelSize = ((pixelSize + 2) / 5) * 5;
  
  loadFontAtSize(pixelSize);
  
  if (!fontLoaded) return;
  
  glm::vec2 pos = positionController.value;
  float x = pos.x * fboPtr->getWidth();
  float y = pos.y * fboPtr->getHeight();
  
  ofRectangle bounds = font.getStringBoundingBox(text, 0, 0);
  x -= bounds.width * 0.5f;   // Center horizontally
  y += bounds.height * 0.5f;  // Center vertically (OF text coords)
  
  ofFloatColor finalColor = colorController.value;
  finalColor.a *= alphaController.value;
  
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  fboPtr->getSource().begin();
  ofSetColor(finalColor);
  font.drawString(text, x, y);
  fboPtr->getSource().end();
}



} // ofxMarkSynth
