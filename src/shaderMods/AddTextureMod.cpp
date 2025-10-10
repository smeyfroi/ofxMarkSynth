//
//  AddTextureMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 30/05/2025.
//

#include "AddTextureMod.hpp"


namespace ofxMarkSynth {


AddTextureMod::AddTextureMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{
  addTextureShader.load();
}

void AddTextureMod::initParameters() {
  parameters.add(scaleParameter);
}

void AddTextureMod::update() {
  assert(false); // not implemented yet
//  if (!addTexture.isAllocated()) return;
//  auto fboPtr = fboPtrs[0];
//  if (fboPtr == nullptr) return;
//  addTextureShader.render(*fboPtr, addTexture, scaleParameter);
}

void AddTextureMod::receive(int sinkId, const float& v) {
  switch (sinkId) {
    case SINK_SCALE:
      scaleParameter = v;
      break;
    default:
      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void AddTextureMod::receive(int sinkId, const ofFloatPixels& pixels) {
  switch (sinkId) {
    case SINK_ADD_PIXELS:
      addTexture.allocate(pixels);
      break;
    default:
      ofLogError() << "ofPixels receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth
