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
  parameters.add(fieldMultiplierParameter);
  parameters.add(fieldBiasParameter);
}

void SmearMod::update() {
  auto fboPtr = fboPtrs[0];
  if (fboPtr == nullptr) return;
  glm::vec2 translation { translateByParameter->x, translateByParameter->y };
  float mixNew = mixNewParameter;
  float alphaMultiplier = alphaMultiplierParameter;
  if (fieldFbo.isAllocated()) {
    smearShader.render(*fboPtr, translation, mixNew, alphaMultiplier, fieldFbo.getTexture(), fieldMultiplierParameter, fieldBiasParameter);
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
      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
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
    case SINK_FIELD_FBO:
      fieldFbo = value;
      break;
    default:
      ofLogError() << "ofFbo receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth
