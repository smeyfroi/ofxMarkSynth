//
//  MultiplyMod.cpp
//  example_fade
//
//  Created by Steve Meyfroidt on 11/05/2025.
//

#include "MultiplyMod.hpp"


namespace ofxMarkSynth {


MultiplyMod::MultiplyMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{
  fadeShader.load();
}

void MultiplyMod::initParameters() {
  parameters.add(multiplyByParameter);
}

void MultiplyMod::update() {
  if (fboPtr == nullptr) return;
  glm::vec4 fade { multiplyByParameter->r, multiplyByParameter->g, multiplyByParameter->b, multiplyByParameter->a };
  fadeShader.render(*fboPtr, fade);
}

void MultiplyMod::receive(int sinkId, const glm::vec4& v) {
  switch (sinkId) {
    case SINK_VEC4:
      multiplyByParameter = v;
      break;
    default:
      ofLogError() << "glm::vec4 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth
