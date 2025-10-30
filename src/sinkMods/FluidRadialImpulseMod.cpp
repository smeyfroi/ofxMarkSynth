//
//  FluidRadialImpulseMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 25/05/2025.
//

#include "FluidRadialImpulseMod.hpp"


namespace ofxMarkSynth {


FluidRadialImpulseMod::FluidRadialImpulseMod(Synth* synthPtr, const std::string& name, const ModConfig&& config)
: Mod { synthPtr, name, std::move(config) }
{
  addRadialImpulseShader.load();
  
  sinkNameIdMap = {
    { "points", SINK_POINTS },
    { "impulseRadius", SINK_IMPULSE_RADIUS },
    { "impulseStrength", SINK_IMPULSE_STRENGTH }
  };
}

void FluidRadialImpulseMod::initParameters() {
  parameters.add(impulseRadiusParameter);
  parameters.add(impulseStrengthParameter);
}

void FluidRadialImpulseMod::update() {
  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto fboPtr = drawingLayerPtrOpt.value()->fboPtr;

  fboPtr->getSource().begin();
  ofEnableBlendMode(OF_BLENDMODE_ADD);
  std::for_each(newPoints.begin(), newPoints.end(), [this, fboPtr](const auto& p) {
    addRadialImpulseShader.render(p * fboPtr->getWidth(),
                                  impulseRadiusParameter * fboPtr->getWidth(),
                                  impulseStrengthParameter);
  });
  fboPtr->getSource().end();
  
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
