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
  auto fboPtr = fboPtrs[0];
  if (fboPtr == nullptr) return;
  fadeShader.render(*fboPtr, multiplyByParameter);
}

void MultiplyMod::receive(int sinkId, const float& v) {
  switch (sinkId) {
    case SINK_FADE:
      multiplyByParameter = v;
      break;
    default:
      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth
