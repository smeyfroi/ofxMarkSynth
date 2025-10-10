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
  auto fboPtrOpt = getNamedFboPtr(DEFAULT_FBOPTR_NAME);
  if (!fboPtrOpt) return;
  auto fboPtr = fboPtrOpt.value();

  glm::vec2 translation { translateByParameter->x, translateByParameter->y };
  translateShader.render(*fboPtr, translation);
}

void TranslateMod::receive(int sinkId, const glm::vec2& v) {
  switch (sinkId) {
    case SINK_VEC2:
      translateByParameter = v;
      break;
    default:
      ofLogError() << "glm::vec2 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth
