//
//  FluidRadialImpulseMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 25/05/2025.
//

#include "FluidRadialImpulseMod.hpp"
#include "IntentMapping.hpp"



namespace ofxMarkSynth {



FluidRadialImpulseMod::FluidRadialImpulseMod(Synth* synthPtr, const std::string& name, const ModConfig&& config)
: Mod { synthPtr, name, std::move(config) }
{
  addRadialImpulseShader.load();
  
  sinkNameIdMap = {
    { "points", SINK_POINTS },
    { impulseRadiusParameter.getName(), SINK_IMPULSE_RADIUS },
    { impulseStrengthParameter.getName(), SINK_IMPULSE_STRENGTH }
  };
  
  sourceNameControllerPtrMap = {
    { impulseRadiusParameter.getName(), &impulseRadiusController },
    { impulseStrengthParameter.getName(), &impulseStrengthController }
  };
}

void FluidRadialImpulseMod::initParameters() {
  parameters.add(impulseRadiusParameter);
  parameters.add(impulseStrengthParameter);
}

void FluidRadialImpulseMod::update() {
  impulseRadiusController.update();
  impulseStrengthController.update();
  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto fboPtr = drawingLayerPtrOpt.value()->fboPtr;

  fboPtr->getSource().begin();
  ofEnableBlendMode(OF_BLENDMODE_ADD);
  std::for_each(newPoints.begin(), newPoints.end(), [this, fboPtr](const auto& p) {
    addRadialImpulseShader.render(p * fboPtr->getWidth(),
                                  impulseRadiusController.value * fboPtr->getWidth(),
                                  impulseStrengthController.value);
  });
  fboPtr->getSource().end();
  
  newPoints.clear();
}

void FluidRadialImpulseMod::receive(int sinkId, const float& value) {
  switch (sinkId) {
    case SINK_IMPULSE_RADIUS:
      impulseRadiusController.updateAuto(value, getAgency());
      break;
    case SINK_IMPULSE_STRENGTH:
      impulseStrengthController.updateAuto(value, getAgency());
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

void FluidRadialImpulseMod::applyIntent(const Intent& intent, float strength) {
  if (strength < 0.01) return;

  // Granularity → Impulse Radius
  float impulseRadiusI = linearMap(intent.getGranularity(), impulseRadiusController);
  impulseRadiusController.updateIntent(impulseRadiusI, strength);
  
  // Energy and Chaos (secondary) → Impulse Strength
  float impulseStrengthI = linearMap(intent.getEnergy(), impulseStrengthController.getManualMin(), impulseStrengthController.getManualMax() * 0.8f) +
      linearMap(intent.getChaos(), 0.0f, impulseStrengthController.getManualMax() * 0.2f);
  impulseStrengthController.updateIntent(impulseStrengthI, strength);
}



} // ofxMarkSynth
