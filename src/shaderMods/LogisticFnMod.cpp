//
//  LogisticFnMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 06/06/2025.
//

#include "LogisticFnMod.hpp"


namespace ofxMarkSynth {


LogisticFnMod::LogisticFnMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{
  logisticFnShader.load();
}

void LogisticFnMod::initParameters() {
  parameters.add(clampFactorParameter);
}

void LogisticFnMod::update() {
  auto fboPtr = fboPtrs[0];
  if (fboPtr == nullptr) return;
  float clampFactor = clampFactorParameter;
  logisticFnShader.render(*fboPtr, glm::vec4(clampFactor));
}


} // ofxMarkSynth
