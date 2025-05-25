//
//  FluidRadialImpulseMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 25/05/2025.
//

#include "FluidRadialImpulseMod.hpp"


namespace ofxMarkSynth {


FluidRadialImpulseMod::FluidRadialImpulseMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{
  addRadialImpulseShader.load();
}

void FluidRadialImpulseMod::initParameters() {
  parameters.add(impulseRadiusParameter);
  parameters.add(impulseStrengthParameter);
}

void FluidRadialImpulseMod::update() {
  auto fboPtr = fboPtrs[0]; // this needs to be the fluid velocities fbo
  if (fboPtr == nullptr) return;
  std::for_each(newPoints.begin(),
                newPoints.end(),
                [this, fboPtr](const auto& p) {
    addRadialImpulseShader.render(*fboPtr, p * fboPtr->getWidth(), impulseRadiusParameter * fboPtr->getWidth(), impulseStrengthParameter);
  });
  newPoints.clear();
}

void FluidRadialImpulseMod::receive(int sinkId, const float& value) {
  switch (sinkId) {
    case SINK_IMPULSE_RADIUS:
      impulseRadiusParameter = value;
      break;
    case SINK_IMPULSE_STRENGTH:
      impulseStrengthParameter = value;
      break;
    default:
      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void FluidRadialImpulseMod::receive(int sinkId, const glm::vec2& point) {
  switch (sinkId) {
    case SINK_POINTS:
      newPoints.push_back(point);
      break;
    default:
      ofLogError() << "glm::vec2 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth
