//
//  TextMod.cpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 18/11/2025.
//

#include "TextMod.hpp"
#include "IntentMapping.hpp"
#include "../IntentMapper.hpp"
#include "Synth.hpp"
#include <algorithm>



namespace ofxMarkSynth {



TextMod::TextMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config, const std::filesystem::path& fontPath_)
: Mod { synthPtr, name, std::move(config) },
  fontPath { fontPath_ }
{
  sinkNameIdMap = {
    { "Text", SINK_TEXT },
    { positionParameter.getName(), SINK_POSITION },
    { fontSizeParameter.getName(), SINK_FONT_SIZE },
    { colorParameter.getName(), SINK_COLOR },
    { alphaParameter.getName(), SINK_ALPHA },
    { drawDurationSecParameter.getName(), SINK_DRAW_DURATION_SEC },
    { alphaFactorParameter.getName(), SINK_ALPHA_FACTOR }
  };

  registerControllerForSource(positionParameter, positionController);
  registerControllerForSource(fontSizeParameter, fontSizeController);
  registerControllerForSource(colorParameter, colorController);
  registerControllerForSource(alphaParameter, alphaController);
  registerControllerForSource(drawDurationSecParameter, drawDurationSecController);
  registerControllerForSource(alphaFactorParameter, alphaFactorController);
}

void TextMod::initParameters() {
  parameters.add(positionParameter);
  parameters.add(fontSizeParameter);
  parameters.add(colorParameter);
  parameters.add(alphaParameter);
  parameters.add(drawDurationSecParameter);
  parameters.add(alphaFactorParameter);
  parameters.add(maxDrawEventsParameter);
  parameters.add(minFontPxParameter);
  parameters.add(agencyFactorParameter);
}

float TextMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void TextMod::update() {
  syncControllerAgencies();

  positionController.update();
  fontSizeController.update();
  colorController.update();
  alphaController.update();
  drawDurationSecController.update();
  alphaFactorController.update();

  if (drawEvents.empty()) return;

  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto drawingLayerPtr = drawingLayerPtrOpt.value();
  auto fboPtr = drawingLayerPtr->fboPtr;
  if (!fboPtr) return;

  float now = ofGetElapsedTimef();
  drawEvents.erase(std::remove_if(drawEvents.begin(), drawEvents.end(), [&](const DrawEvent& e) {
    return (now - e.startTimeSec) >= e.durationSec;
  }), drawEvents.end());
  if (drawEvents.empty()) return;

  fboPtr->getSource().begin();
  ofPushStyle();
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  for (auto& e : drawEvents) {
    drawEvent(e, drawingLayerPtr);
  }
  ofPopStyle();
  fboPtr->getSource().end();
}

void TextMod::receive(int sinkId, const std::string& text) {
  switch (sinkId) {
    case SINK_TEXT:
      ofLogVerbose("TextMod") << "Received text: " << text;
      currentText = text;
      pushDrawEvent(text);
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
    case SINK_DRAW_DURATION_SEC:
      drawDurationSecController.updateAuto(value, getAgency());
      break;
    case SINK_ALPHA_FACTOR:
      alphaFactorController.updateAuto(value, getAgency());
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
  IntentMap im(intent);
  
  im.G().exp(fontSizeController, strength);
  
  // Color composition
  ofFloatColor colorI = energyToColor(intent);
  colorI.a = im.D().get() * 0.5f + 0.5f;
  colorController.updateIntent(colorI, strength, "E->color, D->alpha");
  
  im.D().lin(alphaController, strength);

  // Draw event envelope
  im.G().inv().exp(drawDurationSecController, strength);
  im.D().exp(alphaFactorController, strength);
  
  // Position jitter when chaos > threshold
  float chaos = im.C().get();
  if (chaos > 0.1f) {
    glm::vec2 basePos = positionController.value;
    float jitterAmount = chaos * 0.15f;
    glm::vec2 jitteredPos = basePos + glm::vec2(
      ofRandom(-jitterAmount, jitterAmount),
      ofRandom(-jitterAmount, jitterAmount)
    );
    jitteredPos.x = std::clamp(jitteredPos.x, 0.05f, 0.95f);
    jitteredPos.y = std::clamp(jitteredPos.y, 0.05f, 0.95f);
    positionController.updateIntent(jitteredPos, strength * chaos, "C->jitter");
  }
}

std::shared_ptr<ofTrueTypeFont> TextMod::getFontForSize(int pixelSize) {
  if (pixelSize <= 0) return nullptr;

  auto it = fontCache.find(pixelSize);
  if (it != fontCache.end()) {
    touchFontCacheKey(pixelSize);
    return it->second;
  }

  auto fontPtr = std::make_shared<ofTrueTypeFont>();
  bool success = fontPtr->load(fontPath.string(), pixelSize, true, true);
  if (!success) {
    ofLogError("TextMod") << "Failed to load font from " << fontPath << " at size " << pixelSize;
    return nullptr;
  }

  fontCache[pixelSize] = fontPtr;
  touchFontCacheKey(pixelSize);
  pruneFontCache();

  ofLogNotice("TextMod") << "Loaded font at size " << pixelSize;
  return fontPtr;
}

void TextMod::touchFontCacheKey(int pixelSize) {
  fontCacheLru.erase(std::remove(fontCacheLru.begin(), fontCacheLru.end(), pixelSize), fontCacheLru.end());
  fontCacheLru.push_back(pixelSize);
}

void TextMod::pruneFontCache() {
  while (static_cast<int>(fontCache.size()) > MAX_FONT_CACHE_SIZE) {
    bool evicted = false;

    for (auto it = fontCacheLru.begin(); it != fontCacheLru.end(); ++it) {
      int key = *it;
      auto mapIt = fontCache.find(key);
      if (mapIt == fontCache.end()) {
        fontCacheLru.erase(it);
        evicted = true;
        break;
      }

      if (mapIt->second && mapIt->second.use_count() <= 1) {
        fontCache.erase(mapIt);
        fontCacheLru.erase(it);
        evicted = true;
        break;
      }
    }

    if (!evicted) break;
  }
}

int TextMod::resolvePixelSize(float normalizedFontSize, float fboHeight) const {
  int pixelSize = static_cast<int>(normalizedFontSize * fboHeight);
  pixelSize = std::max(pixelSize, minFontPxParameter.get());

  // Round to nearest 5 pixels to avoid excessive font loads
  pixelSize = ((pixelSize + 2) / 5) * 5;
  return pixelSize;
}

void TextMod::pushDrawEvent(const std::string& text) {
  if (text.empty()) return;

  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto drawingLayerPtr = drawingLayerPtrOpt.value();
  auto fboPtr = drawingLayerPtr->fboPtr;
  if (!fboPtr) return;

  int pixelSize = resolvePixelSize(fontSizeController.value, fboPtr->getHeight());
  auto fontPtr = getFontForSize(pixelSize);
  if (!fontPtr) return;

  DrawEvent e;
  e.text = text;
  e.positionNorm = positionController.value;
  e.baseColor = colorController.value;
  e.baseColor.a *= alphaController.value;
  e.pixelSize = pixelSize;
  e.startTimeSec = ofGetElapsedTimef();
  e.durationSec = std::max(0.001f, drawDurationSecController.value);
  e.alphaFactor = alphaFactorController.value;
  e.applied = 0.0f;
  e.fontPtr = std::move(fontPtr);

  drawEvents.push_back(std::move(e));

  int maxEvents = std::max(1, maxDrawEventsParameter.get());
  while (static_cast<int>(drawEvents.size()) > maxEvents) {
    drawEvents.erase(drawEvents.begin());
  }
}

void TextMod::drawEvent(DrawEvent& e, const DrawingLayerPtr& drawingLayerPtr) {
  if (!drawingLayerPtr || !drawingLayerPtr->fboPtr) return;
  auto fboPtr = drawingLayerPtr->fboPtr;
  if (!e.fontPtr) return;

  float duration = std::max(0.001f, e.durationSec);
  float now = ofGetElapsedTimef();
  float t = (now - e.startTimeSec) / duration;
  t = ofClamp(t, 0.0f, 1.0f);

  float eased = glm::smoothstep(0.0f, 1.0f, t);

  float alphaScale = 0.0f;
  if (drawingLayerPtr->clearOnUpdate) {
    alphaScale = eased;
  } else {
    float target = eased;
    float delta = std::max(0.0f, target - e.applied);

    if (delta <= 0.0f || e.applied >= 0.999f) {
      e.applied = target;
      return;
    }

    float aFrame = delta / std::max(1e-6f, (1.0f - e.applied));
    aFrame = ofClamp(aFrame, 0.0f, 1.0f);
    e.applied = target;
    alphaScale = aFrame;
  }

  ofFloatColor c = e.baseColor;
  c.a *= e.alphaFactor * alphaScale;
  if (c.a <= 0.0f) return;

  float x = e.positionNorm.x * fboPtr->getWidth();
  float y = e.positionNorm.y * fboPtr->getHeight();

  ofRectangle bounds = e.fontPtr->getStringBoundingBox(e.text, 0, 0);
  x -= bounds.width * 0.5f;
  y += bounds.height * 0.5f;

  ofSetColor(c);
  e.fontPtr->drawString(e.text, x, y);
}



} // ofxMarkSynth
