//
//  FluidRadialImpulseMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 25/05/2025.
//

#include "FluidRadialImpulseMod.hpp"
#include "IntentMapping.hpp"
#include "../IntentMapper.hpp"



namespace ofxMarkSynth {



FluidRadialImpulseMod::FluidRadialImpulseMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  addRadialImpulseShader.load();
  
  sinkNameIdMap = {
    { "Point", SINK_POINTS },
    { impulseRadiusParameter.getName(), SINK_IMPULSE_RADIUS },
    { impulseStrengthParameter.getName(), SINK_IMPULSE_STRENGTH }
  };
  
  registerControllerForSource(impulseRadiusParameter, impulseRadiusController);
  registerControllerForSource(impulseStrengthParameter, impulseStrengthController);
}

void FluidRadialImpulseMod::initParameters() {
  parameters.add(impulseRadiusParameter);
  parameters.add(impulseStrengthParameter);
  parameters.add(dtParameter);
  parameters.add(agencyFactorParameter);
}

float FluidRadialImpulseMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void FluidRadialImpulseMod::update() {
  syncControllerAgencies();
  impulseRadiusController.update();
  impulseStrengthController.update();
  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto fboPtr = drawingLayerPtrOpt.value()->fboPtr;

  float dt = dtParameter.get();
  std::for_each(newPoints.begin(), newPoints.end(), [this, dt, fboPtr](const auto& p) {
    addRadialImpulseShader.render(*fboPtr,
                                  p * fboPtr->getWidth(),
                                  impulseRadiusController.value * fboPtr->getWidth(),
                                  impulseStrengthController.value,
                                  dt);
  });
  
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
      ofLogError("FluidRadialImpulseMod") << "Float receive for unknown sinkId " << sinkId;
  }
}

void FluidRadialImpulseMod::receive(int sinkId, const glm::vec2& point) {
  switch (sinkId) {
    case SINK_POINTS:
      newPoints.push_back(point);
      break;
    default:
      ofLogError("FluidRadialImpulseMod") << "glm::vec2 receive for unknown sinkId " << sinkId;
  }
}

void FluidRadialImpulseMod::applyIntent(const Intent& intent, float strength) {
  IntentMap im(intent);

  im.G().lin(impulseRadiusController, strength);
  
  // Weighted blend: energy (80%) + chaos (20%)
  float impulseStrengthI = linearMap(intent.getEnergy(), impulseStrengthController.getManualMin(), impulseStrengthController.getManualMax() * 0.8f) +
      linearMap(intent.getChaos(), 0.0f, impulseStrengthController.getManualMax() * 0.2f);
  impulseStrengthController.updateIntent(impulseStrengthI, strength, "E*.8+C -> lin");
}



} // ofxMarkSynth
