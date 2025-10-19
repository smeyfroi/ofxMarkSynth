//
//  SmearMod.cpp
//  fingerprint-ipad-1
//
//  Created by Steve Meyfroidt on 01/08/2025.
//

#include "SmearMod.hpp"


namespace ofxMarkSynth {


SmearMod::SmearMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{
  smearShader.load();
}

void SmearMod::initParameters() {
  parameters.add(mixNewParameter);
  parameters.add(alphaMultiplierParameter);
  parameters.add(translateByParameter);
  parameters.add(field1MultiplierParameter);
  parameters.add(field1BiasParameter);
  parameters.add(field2MultiplierParameter);
  parameters.add(field2BiasParameter);
}

void SmearMod::update() {
  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto fboPtr = drawingLayerPtrOpt.value()->fboPtr;

  glm::vec2 translation { translateByParameter->x, translateByParameter->y };
  float mixNew = mixNewParameter;
  float alphaMultiplier = alphaMultiplierParameter;
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  // TODO: make this more forgiving
  if (field2Fbo.isAllocated() && field1Fbo.isAllocated()) {
    smearShader.render(*fboPtr, translation, mixNew, alphaMultiplier,
                       field1Fbo.getTexture(), field1MultiplierParameter, field1BiasParameter,
                       field2Fbo.getTexture(), field2MultiplierParameter, field2BiasParameter);
  } else if (field1Fbo.isAllocated()) {
    smearShader.render(*fboPtr, translation, mixNew, alphaMultiplier, field1Fbo.getTexture(), field1MultiplierParameter, field1BiasParameter);
  } else {
    smearShader.render(*fboPtr, translation, mixNew, alphaMultiplier);
  }
}

void SmearMod::receive(int sinkId, const float& v) {
  switch (sinkId) {
    case SINK_FLOAT:
      mixNewParameter = v;
      break;
    default:
      Mod::receive(sinkId, v);
//      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void SmearMod::receive(int sinkId, const glm::vec2& v) {
  switch (sinkId) {
    case SINK_VEC2:
      translateByParameter = v;
      break;
    default:
      ofLogError() << "glm::vec2 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void SmearMod::receive(int sinkId, const ofFbo& value) {
  switch (sinkId) {
    case SINK_FIELD_1_FBO:
      field1Fbo = value;
      break;
    case SINK_FIELD_2_FBO:
      field2Fbo = value;
      break;
    default:
      ofLogError() << "ofFbo receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth
