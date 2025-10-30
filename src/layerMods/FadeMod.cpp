//
//  FadeMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 29/10/2025.
//

#include "FadeMod.hpp"



namespace ofxMarkSynth {



FadeMod::FadeMod(Synth* synthPtr, const std::string& name, const ModConfig&& config)
: Mod { synthPtr, name, std::move(config) }
{
  sinkNameIdMap = {
    { "alphaMultiplier", SINK_ALPHA_MULTIPLIER }
  };
}

void FadeMod::initParameters() {
  parameters.add(alphaMultiplierParameter);
}

void FadeMod::update() {
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
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  ofSetColor(ofFloatColor { 0.0f, 0.0f, 0.0f, alphaMultiplierParameter.get() }); // TODO: fade to a color not just to black
  unitQuadMesh.draw({0.0f, 0.0f}, fboPtr->getSource().getSize());
  fboPtr->getSource().end();
}

void FadeMod::receive(int sinkId, const float& value) {
  switch (sinkId) {
    case SINK_ALPHA_MULTIPLIER:
      alphaMultiplierParameter = value;
      break;
    default:
      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}



} // ofxMarkSynth
