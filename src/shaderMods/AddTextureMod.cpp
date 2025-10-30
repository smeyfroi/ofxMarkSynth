//
//  AddTextureMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 30/05/2025.
//

#include "AddTextureMod.hpp"
#include "IntentMapping.hpp"


namespace ofxMarkSynth {


AddTextureMod::AddTextureMod(Synth* synthPtr, const std::string& name, const ModConfig&& config)
: Mod { synthPtr, name, std::move(config) }
{
  addTextureShader.load();
  
  sinkNameIdMap = {
    { "scale", SINK_SCALE },
    { "addPixels", SINK_ADD_PIXELS }
  };
}

void AddTextureMod::initParameters() {
  parameters.add(scaleParameter);
}

void AddTextureMod::update() {
  scaleController.update();
  assert(false); // not implemented yet
//  if (!addTexture.isAllocated()) return;
//  auto fboPtr = fboPtrs[0];
//  if (fboPtr == nullptr) return;
//  addTextureShader.render(*fboPtr, addTexture, scaleController.value);
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

void AddTextureMod::applyIntent(const Intent& intent, float strength) {
  scaleController.updateIntent(linearMap(intent.getDensity(), 0.02f, 0.2f), strength);
}


} // ofxMarkSynth
