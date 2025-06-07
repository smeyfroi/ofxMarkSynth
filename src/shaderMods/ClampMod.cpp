//
//  ClampMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 07/06/2025.
//

#include "ClampMod.hpp"


namespace ofxMarkSynth {


ClampMod::ClampMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{
  clampShader.load();
}

void ClampMod::initParameters() {
}

void ClampMod::update() {
  auto fboPtr = fboPtrs[0];
  if (fboPtr == nullptr) return;
  clampShader.render(*fboPtr);
}


} // ofxMarkSynth
