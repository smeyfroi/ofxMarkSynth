//
//  TextMod.cpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 18/11/2025.
//

#include "TextMod.hpp"
#include "core/IntentMapping.hpp"
#include "core/IntentMapper.hpp"
#include "core/Synth.hpp"
#include <algorithm>



namespace ofxMarkSynth {



TextMod::TextMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config,
                 std::shared_ptr<FontStash2Cache> fontCache)
: Mod { synthPtr, name, std::move(config) },
  fontCachePtr { fontCache }
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

  // Use config running time (pause-aware) instead of wall clock
  float now = getSynth()->getConfigRunningTime();
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
  
  im.G().exp(fontSizeController, strength, 1.4f);
  
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

int TextMod::resolvePixelSize(float normalizedFontSize, float fboHeight) const {
  int rawSize = static_cast<int>(normalizedFontSize * fboHeight);
  return std::max(rawSize, minFontPxParameter.get());
}

void TextMod::pushDrawEvent(const std::string& text) {
  if (text.empty()) return;
  if (!fontCachePtr) return;

  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto drawingLayerPtr = drawingLayerPtrOpt.value();
  auto fboPtr = drawingLayerPtr->fboPtr;
  if (!fboPtr) return;

  int pixelSize = resolvePixelSize(fontSizeController.value, fboPtr->getHeight());

  DrawEvent e;
  e.text = text;
  e.positionNorm = positionController.value;
  e.baseColor = colorController.value;
  e.baseColor.a *= alphaController.value;
  e.pixelSize = pixelSize;
  // Use config running time (pause-aware) instead of wall clock
  e.startTimeSec = getSynth()->getConfigRunningTime();
  e.durationSec = std::max(0.001f, drawDurationSecController.value);
  e.alphaFactor = alphaFactorController.value;
  e.applied = 0.0f;

  drawEvents.push_back(std::move(e));

  int maxEvents = std::max(1, maxDrawEventsParameter.get());
  while (static_cast<int>(drawEvents.size()) > maxEvents) {
    drawEvents.erase(drawEvents.begin());
  }
}

void TextMod::drawEvent(DrawEvent& e, const DrawingLayerPtr& drawingLayerPtr) {
  if (!drawingLayerPtr || !drawingLayerPtr->fboPtr) return;
  if (!fontCachePtr) return;
  auto fboPtr = drawingLayerPtr->fboPtr;

  float duration = std::max(0.001f, e.durationSec);
  // Use config running time (pause-aware) instead of wall clock
  float now = getSynth()->getConfigRunningTime();
  float t = (now - e.startTimeSec) / duration;
  t = ofClamp(t, 0.0f, 1.0f);

  float alphaScale = 0.0f;
  if (drawingLayerPtr->clearOnUpdate) {
    // For clearing layers (including fluid): quick fade-in (10%), slow fade-out (90%)
    if (t < 0.1f) {
      alphaScale = t / 0.1f;  // 0→1 in first 10%
    } else {
      alphaScale = 1.0f - glm::smoothstep(0.1f, 1.0f, t);  // 1→0 smoothly
    }
  } else {
    // For non-clearing layers: incremental drawing
    float eased = glm::smoothstep(0.0f, 1.0f, t);
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

  // Create style for this draw call
  auto style = fontCachePtr->createStyle(e.pixelSize, c);

  // Get bounding box for centering
  ofRectangle bounds = fontCachePtr->getTextBounds(e.text, style, 0, 0);
  x -= bounds.width * 0.5f;
  y += bounds.height * 0.5f;

  // Draw using ofxFontStash2
  fontCachePtr->draw(e.text, style, x, y);
}



} // ofxMarkSynth
