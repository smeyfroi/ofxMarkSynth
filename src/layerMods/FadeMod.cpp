//
//  FadeMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 29/10/2025.
//

#include "FadeMod.hpp"
#include "core/IntentMapper.hpp"

#include "ofAppRunner.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <string>

namespace ofxMarkSynth {

namespace {

constexpr float FADE_ALPHA_REFERENCE_FPS = 30.0f;

float alphaToHalfLifeSec(float alphaPerFrame, float fps) {
  float alpha = std::clamp(alphaPerFrame, 0.0f, 1.0f - 1e-6f);
  if (alpha <= 0.0f) return std::numeric_limits<float>::infinity();

  float logRemain = std::log1p(-alpha);
  if (!std::isfinite(logRemain) || logRemain >= 0.0f) return std::numeric_limits<float>::infinity();

  float halfLifeFrames = std::log(0.5f) / logRemain;
  return halfLifeFrames / std::max(1e-3f, fps);
}

std::optional<float> tryParseFloat(const std::string& s) {
  try {
    return std::stof(s);
  } catch (...) {
    return std::nullopt;
  }
}

} // namespace


FadeMod::FadeMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config)
: Mod { std::move(synthPtr), name, std::move(config) }
{
  sinkNameIdMap = {
    { halfLifeSecParameter.getName(), SINK_HALF_LIFE_SEC },
    { "Alpha", SINK_ALPHA_LEGACY }
  };

  registerControllerForSource(halfLifeSecParameter, halfLifeSecController);

  // Legacy compatibility: interpret config.Alpha as alpha-per-frame at 30fps.
  if (!this->config.contains(halfLifeSecParameter.getName()) && this->config.contains("Alpha")) {
    if (auto alphaOpt = tryParseFloat(this->config.at("Alpha"))) {
      float halfLifeSec = alphaToHalfLifeSec(*alphaOpt, FADE_ALPHA_REFERENCE_FPS);
      if (std::isfinite(halfLifeSec)) {
        this->config[halfLifeSecParameter.getName()] = std::to_string(halfLifeSec);
      }
    }
    this->config.erase("Alpha");
  }
}

void FadeMod::initParameters() {
  parameters.add(halfLifeSecParameter);
  parameters.add(agencyFactorParameter);
}

float FadeMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void FadeMod::update() {
  syncControllerAgencies();
  halfLifeSecController.update();
  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto fboPtr = drawingLayerPtrOpt.value()->fboPtr;

  // TODO: if we have 8 bit pixels, implement gradual fades to avoid visible remnants that never fully clear
  /**
   void main() {
     vec4 color = texture(tex, texCoordVarying);
     float noise = rand(texCoordVarying + time);

     // Convert to 8-bit integer representation
     vec3 color255 = floor(color.rgb * 255.0 + 0.5);

     // Apply probabilistic fade: chance to reduce by 1/255
     float fadeChance = fadeAmount * 255.0; // Scale fadeAmount to [0,255]
     if (noise < fadeChance && any(greaterThan(color255, vec3(0.0)))) {
       color255 = max(color255 - 1.0, vec3(0.0)); // Decrement, clamped to 0
     }

     fragColor = vec4(color255 / 255.0, color.a);
   }
   */
//    constexpr float min8BitFadeAmount = 1.0f / 255.0f;
//    auto format = fcptr->fboPtr->getSource().getTexture().getTextureData().glInternalFormat;
//    if ((format == GL_RGBA8 || format == GL_RGBA) && fcptr->fadeBy < min8BitFadeAmount) {
//      // FIXME: this doesn't work:
//      // Add dithered noise to alpha to ensure gradual fade completes
//      float dither = ofRandom(0.0f, min8BitFadeAmount);
//      c.a = fcptr->fadeBy + dither;
//    } else {
//      c.a = fcptr->fadeBy;
//    }

  fboPtr->getSource().begin();
  {
    // Fade-to-transparent for premultiplied-alpha layers.
    // Multiply the entire buffer (RGBA) by (1 - fadeAmount) without sampling the texture.
    // This avoids ping-pong FBOs and keeps alpha/RGB consistent.
    float dt = std::clamp(static_cast<float>(ofGetLastFrameTime()), 0.0f, 0.1f);
    float halfLifeSec = std::max(1e-6f, halfLifeSecController.value);
    float mult = std::pow(0.5f, dt / halfLifeSec);
    mult = std::clamp(mult, 0.0f, 1.0f);

    ofPushStyle();
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendColor(mult, mult, mult, mult);
    glBlendFuncSeparate(GL_ZERO, GL_CONSTANT_COLOR, GL_ZERO, GL_CONSTANT_ALPHA);

    ofSetColor(255);
    unitQuadMesh.draw({ 0.0f, 0.0f }, fboPtr->getSource().getSize());

    ofPopStyle();
    glBlendColor(0.0f, 0.0f, 0.0f, 0.0f);
  }
  fboPtr->getSource().end();
}

void FadeMod::receive(int sinkId, const float& value) {
  if (!canDrawOnNamedLayer()) return;

  switch (sinkId) {
    case SINK_HALF_LIFE_SEC:
      halfLifeSecController.updateAuto(value, getAgency());
      break;
    case SINK_ALPHA_LEGACY: {
      float halfLifeSec = alphaToHalfLifeSec(value, FADE_ALPHA_REFERENCE_FPS);
      halfLifeSec = std::clamp(halfLifeSec, halfLifeSecParameter.getMin(), halfLifeSecParameter.getMax());
      halfLifeSecController.updateAuto(halfLifeSec, getAgency());
      break;
    }
    default:
      ofLogError("FadeMod") << "Float receive for unknown sinkId " << sinkId;
  }
}

void FadeMod::applyIntent(const Intent& intent, float strength) {
  IntentMap im(intent);

  // Weighted blend: density (80%) + granularity (20%)
  float densityGranularity = im.D().get() * 0.8f + im.G().get() * 0.2f;

  // Higher density/granularity should generally fade faster (smaller half-life),
  // but keep the adjustment near the tuned baseline.
  Mapping(densityGranularity, "D*.8+G*.2")
      .inv()
      .expAround(halfLifeSecController, strength);
}



} // ofxMarkSynth
