//
//  FadeMod.cpp
//  fingerprint-ipad-1
//
//  Created by Steve Meyfroidt on 25/07/2025.
//

#include "FadeMod.hpp"


namespace ofxMarkSynth {


FadeMod::FadeMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{
//  translateEffect.load();
  fadeEffect.load();
}

void FadeMod::initParameters() {
//  parameters.add(translationParameter);
//  parameters.add(alphaParameter);
  parameters.add(fadeAmountParameter);
}

void FadeMod::update() {
  auto drawingLayerPtrOpt = getNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto fboPtr = drawingLayerPtrOpt.value()->fboPtr;
  
//  glm::vec2 translation { translationParameter->x, translationParameter->y };
//  float alpha = alphaParameter;
//  
//  translateEffect.translateBy = translation;
//  translateEffect.alpha = alpha;
//  translateEffect.draw(*fboPtr);
  
  float fadeAmount = fadeAmountParameter;
  if (fadeAmount != 0.0) {
    // On iOS we have 8 bit pixels, so slow the fade using the minimum 8 bit fade amount at each step to emulate a float fade
    if (fboPtr->getSource().getTexture().getTextureData().glInternalFormat == GL_RGBA) {
      constexpr float min8BitFadeAmount = 1.0 / 128.0; // add one bit of margin
      if (fadeAmount < min8BitFadeAmount) {
        int fadeSteps = ceil(min8BitFadeAmount / fadeAmount);
        if (ofGetFrameNum() % fadeSteps != 0) return;
        fadeAmount = min8BitFadeAmount;
      }
    }
    
    fboPtr->getSource().begin();
    fadeEffect.fadeAmount = fadeAmount;
    fadeEffect.draw(fboPtr->getSource().getWidth(), fboPtr->getSource().getHeight());
    fboPtr->getSource().end();
  }
}

//void FadeMod::receive(int sinkId, const float& v) {
//  switch (sinkId) {
//    case SINK_ALPHA:
//      alphaParameter = v;
//      break;
//    default:
//      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
//  }
//}

//void FadeMod::receive(int sinkId, const glm::vec2& v) {
//  switch (sinkId) {
//    case SINK_TRANSLATION:
//      translationParameter = v;
//      break;
//    default:
//      ofLogError() << "glm::vec2 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
//  }
//}


} // ofxMarkSynth
