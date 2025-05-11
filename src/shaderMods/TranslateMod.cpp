//
//  TranslateMod.cpp
//  example_fade
//
//  Created by Steve Meyfroidt on 11/05/2025.
//

#include "TranslateMod.hpp"


namespace ofxMarkSynth {


TranslateMod::TranslateMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{
  translateShader.load();
}

void TranslateMod::initParameters() {
  parameters.add(translateByParameter);
}

void TranslateMod::update() {
  if (fboPtr == nullptr) return;
  glm::vec2 translation { translateByParameter->x, translateByParameter->y };
  translateShader.render(*fboPtr, translation);
  emit(SOURCE_FBO, fboPtr);
}

void TranslateMod::draw() {
  if (fboPtr == nullptr) return;
  if (hasSinkFor(SOURCE_FBO)) return; // TODO: refactor
  fboPtr->draw(0, 0);
}

void TranslateMod::receive(int sinkId, const FboPtr& fboPtr_) {
  switch (sinkId) {
    case SINK_FBO:
      fboPtr = fboPtr_;
      break;
    default:
      ofLogError() << "PingPongFbo receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void TranslateMod::receive(int sinkId, const glm::vec2& v) {
  switch (sinkId) {
    case SINK_VEC2:
      translateByParameter = v;
      break;
    default:
      ofLogError() << "glm::vec4 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth
